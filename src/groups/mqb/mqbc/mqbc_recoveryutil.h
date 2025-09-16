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

/// @file mqbc_recoveryutil.h
///
/// @brief Provide generic utilities used for recovery operations.
///
/// @bbref{mqbc::RecoveryUtil} provides generic utilities.

// MQB
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
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bsls_types.h>

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

    /// Load corresponding file locations in the specified `journalFd`, the
    /// specified `dataFd`, and the optionally specified `qlistFd` for journal
    /// file, data file, and optionallt Qlist file respectively using the
    /// specified `fileSet`. The function return a return code which is 0 for
    /// success and non-zero otherwise.
    static int loadFileDescriptors(mqbs::MappedFileDescriptor* journalFd,
                                   mqbs::MappedFileDescriptor* dataFd,
                                   const mqbs::FileStoreSet&   fileSet,
                                   mqbs::MappedFileDescriptor* qlistFd = 0);

    /// Validate if the specified `beginSeqNum`, `endSeqNum` and
    /// `destination` are acceptable values which will exhibit defined
    /// behavior.
    static void
    validateArgs(const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
                 const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum,
                 mqbnet::ClusterNode*                         destination);

    /// Skip JOURNAL records by iterating through the specified `journalIt`
    /// until the record after 'beginSeqNum' is reached.  Load this value onto
    /// the specified `currentSeqNum`.
    /// This assumes initial 'journalIt.nextRecord()' call has been done.
    /// The function return zero if successful and non-zero for failure
    /// scenarios.
    static int bootstrapCurrentSeqNum(
        bmqp_ctrlmsg::PartitionSequenceNumber*       currentSeqNum,
        mqbs::JournalFileIterator&                   journalIt,
        const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum);

    /// Increment the specified `currentSeqNum` using the specified
    /// `journalIt`.  Make sure that the incremented sequence number is less
    /// than or equal to the specified `endSeqNum` or else return appropriate
    /// non-zero return code.  Return 0 on successful, 1 if the end of the
    /// journal file is reached, and non-zero for failure scenarios.
    static int incrementCurrentSeqNum(
        bmqp_ctrlmsg::PartitionSequenceNumber* currentSeqNum,
        mqbs::JournalFileIterator&             journalIt);

    /// This function operates on the record currently being pointed by the
    /// specified `journalIt`. It uses the specified `dataFd` if the
    /// record is a data record, the specified `qlistFd` if the record is
    /// a qlist record and the specified `qlistAware` value is true. It
    /// populates the specified `storageMsgType`, `payloadRecordBase` and
    /// `payloadRecordLen` with the appropriate values.
    static void
    processJournalRecord(bmqp::StorageMessageType::Enum*   storageMsgType,
                         char**                            payloadRecordBase,
                         int*                              payloadRecordLen,
                         const mqbs::JournalFileIterator&  journalIt,
                         const mqbs::MappedFileDescriptor& dataFd,
                         bool                              qlistAware,
                         const mqbs::MappedFileDescriptor& qlistFd =
                             mqbs::MappedFileDescriptor());
};

}  // close package namespace
}  // close enterprise namespace

#endif
