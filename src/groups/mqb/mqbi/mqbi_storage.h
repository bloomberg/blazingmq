// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqbi_storage.h                                                     -*-C++-*-
#ifndef INCLUDED_MQBI_STORAGE
#define INCLUDED_MQBI_STORAGE

//@PURPOSE: Provide an interface for a Storage plugin.
//
//@CLASSES:
//  mqbi::Storage:         Interface for the Storage component.
//  mqbi::StorageResult:   Enum of operations result code.
//  mqbi::StorageIterator: Interface for an iterator over the stored items.
//
//@DESCRIPTION: 'mqbi::Storage' is an interface to be implemented by any
// Storage mechanism (inMemory, memoryMap journal, database, ...). The storage
// is used by a queue to keep hold of the messages currently in the queue, and
// must provide a 'mqbi::StorageIterator' implementation to iterate over the
// messages.  Return of each method is one of the 'mqbi::StorageResult' value.
//
/// Thread Safety
///-------------
// Components implementing the 'mqbi::Storage' interface are *NOT* required to
// be thread safe.

// MQB

#include <mqbconfm_messages.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqc_array.h>
#include <bmqc_orderedhashmap.h>
#include <bmqp_protocol.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbu {
class CapacityMeter;
}

namespace mqbi {

// FORWARD DECLARATION
class Queue;
class QueueHandle;

// ====================
// struct StorageResult
// ====================

/// This enum represents the different return code for the put, get and
/// remove operations on a Storage
struct StorageResult {
    // TYPES
    enum Enum {
        e_SUCCESS           = 0,
        e_INVALID_OPERATION = -1,
        e_GUID_NOT_UNIQUE   = -2,
        e_GUID_NOT_FOUND    = -3,
        e_LIMIT_MESSAGES    = -4,
        e_LIMIT_BYTES       = -5,
        e_ZERO_REFERENCES   = -6  // Reference count has gone to zero
        ,
        e_NON_ZERO_REFERENCES = -7  // Reference count is not yet zero
        ,
        e_WRITE_FAILURE    = -8,
        e_APPKEY_NOT_FOUND = -9,
        e_DUPLICATE        = -10
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
    /// `StorageResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&       stream,
                               StorageResult::Enum value,
                               int                 level          = 0,
                               int                 spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(StorageResult::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(StorageResult::Enum*     out,
                          const bslstl::StringRef& str);

    /// Convert the specified `value`, of `StorageResult::Enum` domain to
    /// it's equivalent representation in the `bmqt::AckResult::Enum`
    /// domain.
    static bmqt::AckResult::Enum toAckResult(StorageResult::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, StorageResult::Enum value);

// ==============================
// class StorageMessageAttributes
// ==============================

/// This class provides a VST which captures various attributes associated
/// with a message in the storage.
class StorageMessageAttributes {
  private:
    // DATA
    bsls::Types::Uint64 d_arrivalTimestamp;
    // Arrival timestamp of the message,
    // in seconds from epoch)

    bsls::Types::Int64 d_arrivalTimepoint;
    // Arrival timestamp of the message,
    // in nanoseconds referenced from an
    // arbitrary but fixed point in time.
    // Note that this value is meaningful
    // only in a process.  Also note that
    // zero represents an unset value.

    unsigned int d_refCount;

    /// Unpadded
    unsigned int d_appDataLen;

    bmqp::MessagePropertiesInfo d_messagePropertiesInfo;

    bool d_hasReceipt;

    mqbi::QueueHandle* d_queueHandle;

    unsigned int d_crc32c;
    // CRC32-C associated with this
    // message.

    bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;
    // compression algorithm used to
    // compress this message i.e. the
    // application data.

  public:
    // CLASS METHODS

    /// Write the string representation of the specified
    /// `StorageMessageAttributes` `value` to the specified output `stream`,
    /// and return a reference to `stream`.  Optionally specify an initial
    /// indentation `level`, whose absolute value is incremented recursively
    /// for nested objects.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, whose absolute value indicates the number of
    /// spaces per indentation level for this and all of its nested objects.
    /// If `level` is negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, format the entire output on one line,
    /// suppressing all but the initial indentation (as governed by
    /// `level`).
    static bsl::ostream& print(bsl::ostream&                   stream,
                               const StorageMessageAttributes& value,
                               int                             level,
                               int                             spacesPerLevel);

    // CREATORS
    StorageMessageAttributes();

    StorageMessageAttributes(
        bsls::Types::Uint64                  arrivalTimestamp,
        unsigned int                         appDataLen,
        unsigned int                         refCount,
        const bmqp::MessagePropertiesInfo&   messagePropertiesInfo,
        bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
        bool                                 hasReceipt       = true,
        mqbi::QueueHandle*                   queueHandle      = 0,
        unsigned int                         crc32c           = 0,
        bsls::Types::Int64                   arrivalTimepoint = 0);

    // MANIPULATORS
    StorageMessageAttributes& setArrivalTimestamp(bsls::Types::Uint64 value);
    StorageMessageAttributes& setArrivalTimepoint(bsls::Types::Int64 value);
    StorageMessageAttributes& setRefCount(unsigned int value);
    StorageMessageAttributes& setAppDataLen(unsigned int value);
    StorageMessageAttributes& setCrc32c(unsigned int value);
    StorageMessageAttributes&
    setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::Enum value);
    StorageMessageAttributes& setReceipt(bool value);

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to this object.
    StorageMessageAttributes&
    setMessagePropertiesInfo(const bmqp::MessagePropertiesInfo& value);

    void reset();

    // ACCESSORS
    bsls::Types::Uint64                arrivalTimestamp() const;
    bsls::Types::Int64                 arrivalTimepoint() const;
    unsigned int                       refCount() const;
    unsigned int                       appDataLen() const;
    const bmqp::MessagePropertiesInfo& messagePropertiesInfo() const;
    bool                               hasReceipt() const;
    mqbi::QueueHandle*                 queueHandle() const;

    /// Return the CRC32-C associated with this object.
    unsigned int                         crc32c() const;
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType() const;
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                   stream,
                         const StorageMessageAttributes& value);

/// Compares the specified `lhs` and `rhs` and return `true` if their values
/// are equal or `false` otherwise.
bool operator==(const StorageMessageAttributes& lhs,
                const StorageMessageAttributes& rhs);

/// Compares the specified `lhs` and `rhs` and return `false` if their
/// values are equal or `true` otherwise.
bool operator!=(const StorageMessageAttributes& lhs,
                const StorageMessageAttributes& rhs);

struct AppMessage {
    // VST to track the state associated with (GUID, App) pair.

    // PULIC TYOES
    enum State { e_NONE = 0, e_PUT = 1, e_PUSH = 2, e_CONFIRM = 3 };

    bmqp::RdaInfo d_rdaInfo;
    unsigned int  d_subscriptionId;
    State         d_state;

    AppMessage(const bmqp::RdaInfo& rdaInfo);

    /// Set this object state to indicate a delivery by (Replica or Proxy).
    void setPushState();

    /// Set this object state to indicate a corresponding CONFIRM.
    void setConfirmState();

    /// Set this object state to indicate a corresponding CONFIRM.
    void setRemovedState();

    /// Return `true` if this object is expecting CONFIRM or purge.
    bool isPending() const;

    /// Return `true` if this object is expecting CONFIRM or purge but
    /// had not been sent out (by Replica or Proxy).
    bool isNew() const;

    /// Return `true` if this object is submitted for delivery.
    bool isPushing() const;
};

struct DataStreamMessage {
    // VST to track the state associated with a GUID (for all Apps).

    unsigned d_numApps;
    // number of Apps at the time of this object creation

    const int d_size;
    // The message size

    bsl::vector<mqbi::AppMessage> d_apps;
    // App states for the message

    DataStreamMessage(int numApps, int size, bslma::Allocator* allocator);

    /// Return reference to the modifiable state of the App corresponding
    /// to the specified 'ordinal.
    mqbi::AppMessage& app(unsigned int appOrdinal);

    /// Return reference to the non-modifiable state of the App
    /// corresponding to the specified 'ordinal.
    const mqbi::AppMessage& app(unsigned int appOrdinal) const;
};

// =====================
// class StorageIterator
// =====================

/// Interface for an iterator for items stored in a `mqbi::Storage`.  Each
/// storage must implementation a customized `StorageIterator` associated to
/// it.  Typically, such an implementation should be a small decorator to
/// the underlying structure used by the storage to keep track of the
/// message, hence providing lightweight and very efficient way to iterate
/// over messages.
class StorageIterator {
  public:
    // CREATORS

    /// Destructor
    virtual ~StorageIterator();

    // MANIPULATORS

    /// Clear any cached data associated with this iterator, if any.
    /// The cache might be initialized within `appData`, `options` or
    /// `attributes` routines.
    /// TODO: refactor iterators to remove cached data.
    virtual void clearCache() = 0;

    /// Advance the iterator to the next item. The behavior is undefined
    /// unless `atEnd` returns `false`.  Return `true` if the iterator then
    /// points to a valid item, or `false` if it now is at the end of the
    /// items' collection.
    virtual bool advance() = 0;

    /// If the specified `atEnd` is `true`, reset the iterator to point to the
    /// to the end of the underlying storage.  Otherwise, reset the iterator to
    /// point first item, if any, in the underlying storage.
    virtual void
    reset(const bmqt::MessageGUID& where = bmqt::MessageGUID()) = 0;

    // ACCESSORS

    /// Return a reference offering non-modifiable access to the guid
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    virtual const bmqt::MessageGUID& guid() const = 0;

    /// Return a reference offering non-modifiable access to to the App state
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    virtual const AppMessage&
    appMessageView(unsigned int appOrdinal) const = 0;

    /// Return a reference offering modifiable access to the App state
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    virtual AppMessage& appMessageState(unsigned int appOrdinal) = 0;

    /// Return a reference offering non-modifiable access to the application
    /// data associated with the item currently pointed at by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    virtual const bsl::shared_ptr<bdlbb::Blob>& appData() const = 0;

    /// Return a reference offering non-modifiable access to the options
    /// associated with the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    virtual const bsl::shared_ptr<bdlbb::Blob>& options() const = 0;

    /// Return a reference offering non-modifiable access to the attributes
    /// associated with the message currently pointed at by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    virtual const StorageMessageAttributes& attributes() const = 0;

    /// Return `true` if this iterator is currently at the end of the items'
    /// collection, and hence doesn't reference a valid item.
    virtual bool atEnd() const = 0;

    /// Return `true` if this iterator is currently not at the end of the
    /// `items` collection and the message currently pointed at by this
    /// iterator has received replication factor Receipts.
    virtual bool hasReceipt() const = 0;
};

// =============
// class Storage
// =============

/// Interface for a Storage.
class Storage {
  public:
    // PUBLIC TYPES

    /// `AppInfos` is an alias for an ordered hashtable [appId] -> appKey
    /// The chronological order is for deciding if an app is younger than a
    /// given message (App ordinal < message.refCount).

    typedef bmqc::OrderedHashMap<bsl::string, mqbu::StorageKey> AppInfos;

    typedef bmqc::Array<mqbu::StorageKey,
                        bmqp::Protocol::k_SUBID_ARRAY_STATIC_LEN>
        StorageKeys;

    static const size_t k_INVALID_ORDINAL = 999999;

  public:
    // CREATORS

    /// Destructor
    virtual ~Storage();

    // MANIPULATORS

    /// Configure this storage using the specified `config` and `limits`.
    /// Return 0 on success, or a non-zero return code and fill in a
    /// description of the error in the specified `errorDescription`
    /// otherwise.  Note that calling `configure` on an already configured
    /// storage should atomically reconfigure that storage with the new
    /// configuration (or fail and leave the storage untouched).
    virtual int configure(bsl::ostream&            errorDescription,
                          const mqbconfm::Storage& config,
                          const mqbconfm::Limits&  limits,
                          const bsls::Types::Int64 messageTtl,
                          const int                maxDeliveryAttempts) = 0;

    /// Set the consistency level associated to this storage to the specified
    /// `value`.
    virtual void setConsistency(const mqbconfm::Consistency& value) = 0;

    virtual void setQueue(mqbi::Queue* queue) = 0;

    /// Close this storage.
    virtual void close() = 0;

    /// Get an iterator for data stored in the virtual storage identified by
    /// the specified `appKey`.
    /// If the `appKey` is null, the returned  iterator can iterate states of
    /// all Apps; otherwise, the iterator can iterate states of the App
    /// corresponding to the `appKey`.
    virtual bslma::ManagedPtr<StorageIterator>
    getIterator(const mqbu::StorageKey& appKey) = 0;

    /// Load into the specified `out` an iterator for data stored in the
    /// virtual storage initially pointing to the message associated with the
    /// specified `msgGUID`.
    /// If the `appKey` is null, the returned  iterator can iterate states of
    /// all Apps; otherwise, the iterator can iterate states of the App
    /// corresponding to the `appKey`.
    /// Return zero on success, and a non-zero code if `msgGUID` was not
    /// found in the storage.
    virtual StorageResult::Enum
    getIterator(bslma::ManagedPtr<StorageIterator>* out,
                const mqbu::StorageKey&             appKey,
                const bmqt::MessageGUID&            msgGUID) = 0;

    /// Save the message contained in the specified `appData`, `options` and
    /// the associated `attributes` and `msgGUID` into this storage and the
    /// associated virtual storage.  The `attributes` is an in/out parameter
    /// and storage layer can populate certain fields of that struct.  If the
    /// optionally specified `out` is not zero, load the created
    /// `DataStreamMessage` into the 'out'.
    /// Return 0 on success or an non-zero error code on failure.
    virtual StorageResult::Enum
    put(StorageMessageAttributes*           attributes,
        const bmqt::MessageGUID&            msgGUID,
        const bsl::shared_ptr<bdlbb::Blob>& appData,
        const bsl::shared_ptr<bdlbb::Blob>& options,
        mqbi::DataStreamMessage**           out = 0) = 0;

    /// Update the App state corresponding to the specified `msgGUID` and the
    /// specified `appKey` in the DataStream.  Decrement the reference count of
    /// the message identified by the `msgGUID`, and record the CONFIRM in the
    /// storage.
    /// Return one of the return codes from:
    /// * e_SUCCESS          : success
    /// * e_GUID_NOT_FOUND      : `msgGUID` was not found
    /// * e_ZERO_REFERENCES     : message refCount has become zero
    /// * e_NON_ZERO_REFERENCES : message refCount is still not zero
    /// * e_WRITE_FAILURE       : failed to record this event in storage
    ///
    /// Behavior is undefined unless there is an App with the `appKey`.
    ///
    /// On CONFIRM, the caller of `confirm` is responsible to follow with
    /// `remove` call.  `releaseRef` is an alternative way to remove message in
    /// one call.
    virtual StorageResult::Enum confirm(const bmqt::MessageGUID& msgGUID,
                                        const mqbu::StorageKey&  appKey,
                                        bsls::Types::Int64       timestamp,
                                        bool onReject = false) = 0;

    /// Decrement the reference count of the message identified by the
    /// `msgGUID`.  If the resulting value is zero and the specified
    /// `asPrimary` is `true`, delete the message data and record the event in
    /// the storage.
    /// Return one of the return codes from:
    /// * e_SUCCESS          : success
    /// * e_GUID_NOT_FOUND      : `msgGUID` was not found
    /// * e_INVALID_OPERATION   : the value is invalid (already zero)
    /// * e_ZERO_REFERENCES     : message refCount has become zero
    /// * e_NON_ZERO_REFERENCE  : message refCount is still not zero
    ///
    /// On CONFIRM, the caller of `confirm` is responsible to follow with
    /// `remove` call.  `releaseRef` is an alternative way to remove message in
    /// one call.
    virtual StorageResult::Enum releaseRef(const bmqt::MessageGUID& msgGUID,
                                           bool asPrimary = true) = 0;

    /// Remove from the storage the message having the specified `msgGUID`
    /// and store it's size, in bytes, in the optionally specified `msgSize`.
    /// Record the event in the storage.
    /// Return 0 on success, or a non-zero return code if the `msgGUID` was not
    /// found or if has failed to record this event in storage.
    ///
    /// On CONFIRM, the caller of `confirm` is responsible to follow with
    /// `remove` call.  `releaseRef` is an alternative way to remove message in
    /// one call.
    virtual StorageResult::Enum remove(const bmqt::MessageGUID& msgGUID,
                                       int* msgSize = 0) = 0;

    /// Remove all messages from this storage for the App identified by the
    /// specified `appKey` if `appKey` is not null.  Otherwise, remove messages
    /// for all Apps.  Record the event in the storage.
    /// Return one of the return codes from:
    /// * e_SUCCESS          : success
    /// * e_WRITE_FAILURE    : failed to record this event in storage
    /// * e_APPKEY_NOT_FOUND : Invalid `appKey` specified
    virtual StorageResult::Enum removeAll(const mqbu::StorageKey& appKey) = 0;

    /// Flush any buffered replication messages to the peers.  Behaviour is
    /// undefined unless this cluster node is the primary for this partition.
    virtual void flushStorage() = 0;

    /// Return the resource capacity meter associated to this storage.
    virtual mqbu::CapacityMeter* capacityMeter() = 0;

    /// Attempt to garbage-collect messages for which TTL has expired, and
    /// return the number of messages garbage-collected.  Populate the
    /// specified `latestGcMsgTimestampEpoch` with the timestamp, as seconds
    /// from epoch, of the oldest encountered message, and the specified
    /// `configuredTtlValue` with the TTL value (in seconds) with which this
    /// storage instance is configured.
    virtual int gcExpiredMessages(bsls::Types::Uint64* latestMsgTimestampEpoch,
                                  bsls::Types::Int64*  configuredTtlValue,
                                  bsls::Types::Uint64  secondsFromEpoch) = 0;

    /// Garbage-collect those messages from the deduplication history which
    /// have expired the deduplication window.  Return `true`, if there are
    /// expired items unprocessed because of the batch limit.
    virtual bool gcHistory() = 0;

    /// Create, if it doesn't exist already, a virtual storage instance with
    /// the specified `appId` and `appKey`.  Return zero upon success and a
    /// non-zero value otherwise, and populate the specified
    /// `errorDescription` with a brief reason in case of failure.  Behavior
    /// is undefined unless `appId` is non-empty and `appKey` is non-null.
    virtual int addVirtualStorage(bsl::ostream&           errorDescription,
                                  const bsl::string&      appId,
                                  const mqbu::StorageKey& appKey) = 0;

    /// Remove the virtual storage identified by the specified `appKey`.  The
    /// specified `asPrimary` indicates if this storage need to write Purge
    /// record in the case of persistent storage.
    /// Return true if a virtual storage with `appKey` was found and
    /// deleted, false if a virtual storage with `appKey` does not exist.
    /// Behavior is undefined unless `appKey` is non-null.  Note that this
    /// method will delete the virtual storage, and any reference to it will
    /// become invalid after this method returns.
    virtual bool removeVirtualStorage(const mqbu::StorageKey& appKey,
                                      bool                    asPrimary) = 0;

    virtual void selectForAutoConfirming(const bmqt::MessageGUID& msgGUID) = 0;
    virtual StorageResult::Enum autoConfirm(const mqbu::StorageKey& appKey,
                                            bsls::Types::Uint64 timestamp) = 0;
    /// The sequence of calls is `selectForAutoConfirming`, then zero or more
    /// `autoConfirm`, then `put` - all for the same specified `msgGUID`.
    /// `autoConfirm` replicates ephemeral auto CONFIRM for the specified
    /// `appKey` in persistent storage.
    /// Any other sequence removes auto CONFIRMs.
    /// Auto-confirmed Apps do not PUSH the message.

    // ACCESSORS

    /// Return the queue this storage is associated with.
    virtual mqbi::Queue* queue() const = 0;

    /// Return the URI of the queue this storage is associated with.
    virtual const bmqt::Uri& queueUri() const = 0;

    /// Return the storage key associated with this instance.
    virtual const mqbu::StorageKey& queueKey() const = 0;

    /// Return the current configuration used by this storage. The behavior
    /// is undefined unless `configure` was successfully called.
    virtual const mqbconfm::Storage& config() const = 0;

    /// Return the partitionId associated with this storage.
    virtual int partitionId() const = 0;

    /// Return true if storage is backed by a persistent data store,
    /// otherwise return false.
    virtual bool isPersistent() const = 0;

    /// Return the number of messages in the virtual storage associated with
    /// the specified `appKey`. If `appKey` is null, number of messages in
    /// the `physical` storage is returned.  Behavior is undefined if
    /// `appKey` is non-null but no virtual storage identified with it
    /// exists.
    virtual bsls::Types::Int64
    numMessages(const mqbu::StorageKey& appKey) const = 0;

    /// Return the number of bytes in the virtual storage associated with
    /// the specified `appKey`. If `appKey` is null, number of bytes in the
    /// `physical` storage is returned. Behavior is undefined if
    /// `appKey` is non-null but no virtual storage identified with it
    /// exists.
    virtual bsls::Types::Int64
    numBytes(const mqbu::StorageKey& appKey) const = 0;

    /// Return true if storage is empty.  This method can be invoked from
    /// any thread.
    virtual bool isEmpty() const = 0;

    /// Return true if this storage has message with the specified
    /// `msgGUID`, false otherwise.
    virtual bool hasMessage(const bmqt::MessageGUID& msgGUID) const = 0;

    /// Retrieve the message and its metadata having the specified `msgGUID`
    /// in the specified `appData`, `options` and `attributes` from this
    /// storage.  Return zero on success or a non-zero error code on
    /// failure.
    virtual StorageResult::Enum
    get(bsl::shared_ptr<bdlbb::Blob>* appData,
        bsl::shared_ptr<bdlbb::Blob>* options,
        StorageMessageAttributes*     attributes,
        const bmqt::MessageGUID&      msgGUID) const = 0;

    /// Populate the specified `attributes` buffer with attributes of the
    /// message having the specified `msgGUID`.  Return zero on success or a
    /// non-zero error code on failure.
    virtual StorageResult::Enum
    get(StorageMessageAttributes* attributes,
        const bmqt::MessageGUID&  msgGUID) const = 0;

    /// Store in the specified `msgSize` the size, in bytes, of the message
    /// having the specified `msgGUID` if found and return success, or
    /// return a non-zero return code and leave `msgSize` untouched if no
    /// message with `msgGUID` were found.
    virtual StorageResult::Enum
    getMessageSize(int* msgSize, const bmqt::MessageGUID& msgGUID) const = 0;

    /// Return the number of virtual storages registered with this instance.
    virtual int numVirtualStorages() const = 0;

    /// Return true if virtual storage identified by the specified `appKey`
    /// exists, otherwise return false.  Load into the optionally specified
    /// `appId` the appId associated with `appKey` if the virtual storage
    /// exists, otherwise set it to 0.
    virtual bool hasVirtualStorage(const mqbu::StorageKey& appKey,
                                   bsl::string* appId = 0) const = 0;

    /// Return true if virtual storage identified by the specified `appId`
    /// exists, otherwise return false.  Load into the optionally specified
    /// `appKey` and `ordinal` the appKey and ordinal associated with `appId`
    /// if the virtual storage exists, otherwise set it to 0.
    virtual bool hasVirtualStorage(const bsl::string& appId,
                                   mqbu::StorageKey*  appKey  = 0,
                                   unsigned int*      ordinal = 0) const = 0;

    /// Return `true` if there was Replication Receipt for the specified
    /// `msgGUID`.
    virtual bool hasReceipt(const bmqt::MessageGUID& msgGUID) const = 0;

    /// Load into the specified `buffer` the list of pairs of appId and
    /// appKey for all the virtual storages registered with this instance.
    virtual void loadVirtualStorageDetails(AppInfos* buffer) const = 0;

    /// Return the number of auto confirmed Apps for the current message.
    virtual unsigned int numAutoConfirms() const = 0;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// class AppMessage
// ----------------

inline AppMessage::AppMessage(const bmqp::RdaInfo& rdaInfo)
: d_rdaInfo(rdaInfo)
, d_subscriptionId(bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID)
, d_state(e_PUT)
{
    // NOTHING
}

inline void AppMessage::setPushState()
{
    d_state = e_PUSH;
}

inline void AppMessage::setConfirmState()
{
    d_state = e_CONFIRM;
}

inline void AppMessage::setRemovedState()
{
    d_state = e_NONE;
}

inline bool AppMessage::isPending() const
{
    return d_state == e_PUT || d_state == e_PUSH;
}

inline bool AppMessage::isNew() const
{
    return d_state == e_PUT;
}

inline bool AppMessage::isPushing() const
{
    return d_state == e_PUSH;
}

// -----------------------
// class DataStreamMessage
// -----------------------

inline DataStreamMessage::DataStreamMessage(int               numApps,
                                            int               size,
                                            bslma::Allocator* allocator)
: d_numApps(numApps)
, d_size(size)
, d_apps(allocator)
{
    // NOTHING
}

inline mqbi::AppMessage& DataStreamMessage::app(unsigned int appOrdinal)
{
    BSLS_ASSERT_SAFE(appOrdinal < d_apps.size());

    return d_apps[appOrdinal];
}

inline const mqbi::AppMessage&
DataStreamMessage::app(unsigned int appOrdinal) const
{
    BSLS_ASSERT_SAFE(appOrdinal < d_apps.size());

    return d_apps[appOrdinal];
}

// ------------------------------
// class StorageMessageAttributes
// ------------------------------

// CREATORS
inline StorageMessageAttributes::StorageMessageAttributes()
: d_arrivalTimestamp(0)
, d_arrivalTimepoint(0)
, d_refCount(0)
, d_appDataLen(0)
, d_messagePropertiesInfo()
, d_hasReceipt(true)
, d_queueHandle(0)
, d_crc32c(0)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
{
}

inline StorageMessageAttributes::StorageMessageAttributes(
    bsls::Types::Uint64                  arrivalTimestamp,
    unsigned int                         refCount,
    unsigned int                         appDataLen,
    const bmqp::MessagePropertiesInfo&   messagePropertiesInfo,
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
    bool                                 hasReceipt,
    mqbi::QueueHandle*                   queueHandle,
    unsigned int                         crc32c,
    bsls::Types::Int64                   arrivalTimepoint)
: d_arrivalTimestamp(arrivalTimestamp)
, d_arrivalTimepoint(arrivalTimepoint)
, d_refCount(refCount)
, d_appDataLen(appDataLen)
, d_messagePropertiesInfo(messagePropertiesInfo)
, d_hasReceipt(hasReceipt)
, d_queueHandle(queueHandle)
, d_crc32c(crc32c)
, d_compressionAlgorithmType(compressionAlgorithmType)
{
    // NOTHING
}

// MANIPULATORS
inline StorageMessageAttributes&
StorageMessageAttributes::setArrivalTimestamp(bsls::Types::Uint64 value)
{
    d_arrivalTimestamp = value;
    return *this;
}

inline StorageMessageAttributes&
StorageMessageAttributes::setArrivalTimepoint(bsls::Types::Int64 value)
{
    d_arrivalTimepoint = value;
    return *this;
}

inline StorageMessageAttributes&
StorageMessageAttributes::setRefCount(unsigned int value)
{
    d_refCount = value;
    return *this;
}

inline StorageMessageAttributes&
StorageMessageAttributes::setAppDataLen(unsigned int value)
{
    d_appDataLen = value;
    return *this;
}

inline StorageMessageAttributes&
StorageMessageAttributes::setCrc32c(unsigned int value)
{
    d_crc32c = value;
    return *this;
}

inline StorageMessageAttributes&
StorageMessageAttributes::setCompressionAlgorithmType(
    bmqt::CompressionAlgorithmType::Enum value)
{
    d_compressionAlgorithmType = value;
    return *this;
}

inline StorageMessageAttributes&
StorageMessageAttributes::setReceipt(bool value)
{
    d_hasReceipt = value;
    return *this;
}

inline StorageMessageAttributes&
StorageMessageAttributes::setMessagePropertiesInfo(
    const bmqp::MessagePropertiesInfo& value)
{
    d_messagePropertiesInfo = value;
    return *this;
}

inline void StorageMessageAttributes::reset()
{
    d_arrivalTimestamp         = 0;
    d_arrivalTimepoint         = 0;
    d_refCount                 = 0;
    d_appDataLen               = 0;
    d_messagePropertiesInfo    = bmqp::MessagePropertiesInfo();
    d_queueHandle              = 0;
    d_hasReceipt               = true;
    d_crc32c                   = 0;
    d_compressionAlgorithmType = bmqt::CompressionAlgorithmType::e_NONE;
}

// ACCESSORS
inline bsls::Types::Uint64 StorageMessageAttributes::arrivalTimestamp() const
{
    return d_arrivalTimestamp;
}

inline bsls::Types::Int64 StorageMessageAttributes::arrivalTimepoint() const
{
    return d_arrivalTimepoint;
}

inline unsigned int StorageMessageAttributes::refCount() const
{
    return d_refCount;
}

inline unsigned int StorageMessageAttributes::appDataLen() const
{
    return d_appDataLen;
}

inline const bmqp::MessagePropertiesInfo&
StorageMessageAttributes::messagePropertiesInfo() const
{
    return d_messagePropertiesInfo;
}

inline bool StorageMessageAttributes::hasReceipt() const
{
    return d_hasReceipt;
}

inline mqbi::QueueHandle* StorageMessageAttributes::queueHandle() const
{
    return d_queueHandle;
}

inline unsigned int StorageMessageAttributes::crc32c() const
{
    return d_crc32c;
}

inline bmqt::CompressionAlgorithmType::Enum
StorageMessageAttributes::compressionAlgorithmType() const
{
    return d_compressionAlgorithmType;
}

// FREE OPERATORS
inline bsl::ostream& operator<<(bsl::ostream&                   stream,
                                const StorageMessageAttributes& value)
{
    return StorageMessageAttributes::print(stream, value, 0, -1);
}

inline bool operator==(const StorageMessageAttributes& lhs,
                       const StorageMessageAttributes& rhs)
{
    return lhs.arrivalTimestamp() == rhs.arrivalTimestamp() &&
           lhs.arrivalTimepoint() == rhs.arrivalTimepoint() &&
           lhs.refCount() == rhs.refCount() &&
           lhs.appDataLen() == rhs.appDataLen() &&
           lhs.messagePropertiesInfo() == rhs.messagePropertiesInfo() &&
           lhs.crc32c() == rhs.crc32c() &&
           lhs.compressionAlgorithmType() == rhs.compressionAlgorithmType();
}

inline bool operator!=(const StorageMessageAttributes& lhs,
                       const StorageMessageAttributes& rhs)
{
    return !(lhs == rhs);
}

}  // close package namespace

namespace bmqu {

bsl::ostream&
operator<<(bsl::ostream&                                 stream,
           const bmqu::Printer<mqbi::Storage::AppInfos>& printer);

}

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// struct StorageResult
// --------------------

inline bsl::ostream& mqbi::operator<<(bsl::ostream&             stream,
                                      mqbi::StorageResult::Enum value)
{
    return mqbi::StorageResult::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
