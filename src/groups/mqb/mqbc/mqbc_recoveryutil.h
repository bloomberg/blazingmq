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

// mqbc_recoveryutil.h                                                -*-C++-*-
#ifndef INCLUDED_MQBC_RECOVERYUTIL
#define INCLUDED_MQBC_RECOVERYUTIL

//@PURPOSE: Provide generic utilities used for recovery operations.
//
//@CLASSES:
//  mqbc::StorageUtil: Generic utilities for recovery related operations.
//
//@DESCRIPTION: 'mqbc::RecoveryUtil' provides generic utilities.

// MQB

#include <mqbc_clusterdata.h>
#include <mqbi_storage.h>
#include <mqbnet_cluster.h>
#include <mqbs_filestore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bsls_types.h>
#include <bslstl_string.h>
#include <bslstl_vector.h>

namespace BloombergLP {

namespace mqbc {
// ===================
// struct RecoveryUtil
// ===================

/// Generic utilities for recovery related operations.
struct RecoveryUtil {
  private:
    // CLASS SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.RECOVERYUTIL");

  public:
    // FUNCTIONS

    /// Load corresponding file locations in the specified `journalFd` and
    /// the specified `dataFd` for journal file and data file respectively
    /// using the specified `fileSet`. The function return a return code
    /// which is 0 for success and non-zero otherwise.
    static int loadFileDescriptors(mqbs::MappedFileDescriptor* journalFd,
                                   mqbs::MappedFileDescriptor* dataFd,
                                   const mqbs::FileStoreSet&   fileSet);

    /// Load the begin and end offsets from specified `journalFd` into the
    /// specified `journalFileBeginOffset`, specified `journalFileEndOffset`.
    /// Load the begin and end offsets from specified `dataFd` into the
    /// specified `dataFileBeginOffset`, specified `dataFileEndOffset`. Set
    /// the specified `isRollover` flag if pointing to a rolled over file.
    /// Use the specified `beginSeqNum` and specified `endSeqNum` as given
    /// by the caller to determine range of required records. The function
    /// returns a return code which is 0 for success and non-zero otherwise.
    static int
    loadOffsets(bsls::Types::Uint64*              journalFileBeginOffset,
                bsls::Types::Uint64*              journalFileEndOffset,
                bsls::Types::Uint64*              dataFileBeginOffset,
                bsls::Types::Uint64*              dataFileEndOffset,
                bool*                             isRollover,
                const mqbs::MappedFileDescriptor& journalFd,
                const mqbs::MappedFileDescriptor& dataFd,
                const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
                const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum);

    /// Validate if the specified `beginSeqNum`, `endSeqNum` and
    /// `destination` are acceptable values which will exhibit defined
    /// behavior.
    static void
    validateArgs(const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
                 const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum,
                 mqbnet::ClusterNode*                         destination);

    /// Get the current sequence number by iterating through `journalIt`
    /// until we reach the specified `beginSeqNum`. Load this value onto the
    /// specified `currentSeqNum`.
    /// The function return zero if successful and non-zero for failure
    /// scenarios.
    static int bootstrapCurrentSeqNum(
        bmqp_ctrlmsg::PartitionSequenceNumber*       currentSeqNum,
        mqbs::JournalFileIterator&                   journalIt,
        const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum);

    /// This function increments the specified `currentSeqNum` using the
    /// specified `journalRecordBase` as per the specified `journalFd`.
    /// The function makes sure that the incremented sequence number is less
    /// than or equal to the specified `endSeqNum` or else it returns
    /// appropriate non-zero return code.
    /// This operation is performed for the specified `partitionId`, the
    /// specified `destination` node and the specified `clusterData` is
    /// used for logging purposes. The specified `journalIt` is used.
    /// The function return 0 if successful, 1 if end of journal file is
    /// reached, and non-zero for failure scenarios.
    static int incrementCurrentSeqNum(
        bmqp_ctrlmsg::PartitionSequenceNumber*       currentSeqNum,
        char**                                       journalRecordBase,
        const mqbs::MappedFileDescriptor&            journalFd,
        const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum,
        int                                          partitionId,
        const mqbnet::ClusterNode&                   destination,
        const mqbc::ClusterData&                     clusterData,
        mqbs::JournalFileIterator&                   journalIt);

    /// This function operates on the record currently being pointed by the
    /// specified `journalIt`. It uses the specified `dataFd` if the
    /// record is a data record, the specified `qlistFd` if the record is
    /// a qlist record and the specified `fsmWorkflow` value is false. It
    /// populates the specified `storageMsgType`, `payloadRecordBase` and
    /// `payloadRecordLen` with the appropriate values.
    static void
    processJournalRecord(bmqp::StorageMessageType::Enum*   storageMsgType,
                         char**                            payloadRecordBase,
                         int*                              payloadRecordLen,
                         const mqbs::JournalFileIterator&  journalIt,
                         const mqbs::MappedFileDescriptor& dataFd,
                         bool                              fsmWorkflow,
                         const mqbs::MappedFileDescriptor& qlistFd =
                             mqbs::MappedFileDescriptor());
};

}  // close package namespace
}  // close enterprise namespace

#endif
