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

// mqbs_filestoreprotocolutil.h                                       -*-C++-*-
#ifndef INCLUDED_MQBS_FILESTOREPROTOCOLUTIL
#define INCLUDED_MQBS_FILESTOREPROTOCOLUTIL

//@PURPOSE: Provide utilities for BlazingMQ file store protocol.
//
//@CLASSES:
//  mqbs::FileStoreProtocolUtil: Utilities for BlazingMQ file store protocol
//
//@SEE ALSO: mqbs::FileStoreProtcol
//
//@DESCRIPTION: 'mqbs::FileStoreProtocolUtil' provides utilities for BlazingMQ
// file store protocol, in the 'mqbs::FileStoreProtocolUtil' namespace.

// MQB

#include <mqbconfm_messages.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbu_storagekey.h>

#include <bmqu_blob.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlde_md5.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

// ===========================
// class FileStoreProtocolUtil
// ===========================

/// This component provides utilities for BlazingMQ file store protocol.
struct FileStoreProtocolUtil {
    // CLASS METHODS

    /// Return zero if the file represented by the specified `mfd` contains
    /// BlazingMQ file store protocol header, non-zero value otherwise.
    static int hasBmqHeader(const MappedFileDescriptor& mfd);

    /// Return the BlazingMQ file store protocol header contained in the
    /// file represented by the specified `mfd`.  The behavior is undefined
    /// unless `hasBmqHeader` returns success.
    static const FileHeader& bmqHeader(const MappedFileDescriptor& mfd);

    /// Return the position of last valid journal sync point in the journal
    /// file represented by the specified `mfd` and having the specified
    /// `fileHeader` and `journalHeader` headers.  Return zero if no sync
    /// point was found.  Note that return value of zero does not indicate a
    /// corrupted journal; it indicates that journal may be small enough not
    /// to contain any sync point.  Also note that if the `journalHeader`
    /// indicates that journal does not contain any sync points, then zero
    /// is returned.
    static bsls::Types::Uint64
    lastJournalSyncPoint(const MappedFileDescriptor& mfd,
                         const FileHeader&           fileHeader,
                         const JournalFileHeader&    journalHeader);

    /// Return the position of last valid record in the journal file
    /// represented by the specified `mfd` and having the specified
    /// `fileHeader` and `journalHeader` headers, and last valid sync point
    /// position indicated by the specified `lastJournalSyncPoint`.  Return
    /// zero if no valid record is present.  Note that last journal sync
    /// point could be the last valid record in the journal, and in that
    /// case, `lastJournalSyncPoint` will be returned.
    static bsls::Types::Uint64
    lastJournalRecord(const MappedFileDescriptor& mfd,
                      const FileHeader&           fileHeader,
                      const JournalFileHeader&    journalHeader,
                      bsls::Types::Uint64         lastJournalSyncPoint);

    /// Return zero if the journal file represented by the specified
    /// `journalFd` contains a valid and non-null first sync point record,
    /// non-zero value otherwise.
    static int
    hasValidFirstSyncPointRecord(const MappedFileDescriptor& journalFd);

    /// Load into the specified `buffer` the MD5 digest of the section of
    /// the specified `blob` of the specified `length` starting at the
    /// specified `startPos` position.  Return zero on success, non-zero
    /// value otherwise.  Behavior is undefined unless `buffer` is non null.
    /// Behavior is also undefined unless `startPos` represents a valid
    /// position in the `blob` and `length` is non-zero.
    static int calculateMd5Digest(bdlde::Md5::Md5Digest*    buffer,
                                  const bdlbb::Blob&        blob,
                                  const bmqu::BlobPosition& startPos,
                                  unsigned int              length);

    static void loadAppIdKeyPairs(
        bsl::vector<bsl::pair<bsl::string, mqbu::StorageKey> >* appIdKeyPairs,
        const MemoryBlock&                                      appIdsBlock,
        unsigned int                                            numAppIds);
};

}  // close package namespace
}  // close enterprise namespace

#endif
