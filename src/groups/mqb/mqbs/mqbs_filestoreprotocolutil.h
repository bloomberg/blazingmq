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
#include <mqbi_storage.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbu_storagekey.h>

#include <bmqu_blob.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlde_md5.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
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
    /// `journalFd` contains a valid and non-null first rollover sync point
    /// record, non-zero value otherwise.
    static int hasValidFirstRolloverSyncPointRecord(
        const MappedFileDescriptor& journalFd);

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

    /// @brief Derive the byte lengths of the sections of a queue record.
    ///
    /// @details A QLIST queue record is laid out as `[QueueRecordHeader]
    /// [Padded QueueUri][QueueUri Hash][AppId entries...][Magic word]`, and
    /// the specified `header` declares the length of each of those sections
    /// as well as of the record as a whole.  The caller remains responsible
    /// for confirming that the record fits the blob or file holding it.
    ///
    /// @param[out] headerLen     Size of the `QueueRecordHeader`.
    /// @param[out] paddedUriLen  Size of the padded queue URI.
    /// @param[out] appIdsAreaLen Size of the application-ID area.
    /// @param header             Header to derive the lengths from.
    /// @returns 0 if the sections declared by `header` fit within the record
    ///          length it declares, a non-zero value otherwise.
    static int loadQueueRecordLayout(unsigned int*            headerLen,
                                     unsigned int*            paddedUriLen,
                                     unsigned int*            appIdsAreaLen,
                                     const QueueRecordHeader& header);

    /// @brief Derive the unpadded length of a WORD-padded field.
    ///
    /// @param[out] length   Length of the field excluding its padding.
    /// @param data          First byte of the padded field.
    /// @param paddedLength  Length of the field including its padding.
    /// @returns 0 if the trailing padding byte of the field is valid, a
    ///          non-zero value otherwise.
    ///
    /// The behavior is undefined unless `paddedLength` bytes are readable at
    /// `data`.
    static int loadUnpaddedLength(unsigned int* length,
                                  const char*   data,
                                  unsigned int  paddedLength);

    /// @brief Return true if a queue record ends with the queue record magic
    ///        word.
    ///
    /// @param block        Memory holding the record.
    /// @param recordOffset Offset of the record within `block`.
    /// @param recordLen    Length of the record.
    ///
    /// The behavior is undefined unless `block` holds `recordLen` bytes at
    /// `recordOffset`, and `recordLen` is at least the size of a magic word.
    static bool hasValidQueueRecordMagic(const MemoryBlock&  block,
                                         bsls::Types::Uint64 recordOffset,
                                         unsigned int        recordLen);

    /// @brief Load the appId/appKey pairs of a queue record.
    ///
    /// @param[out] appIdKeyPairs Pairs read from `appIdsBlock`.
    /// @param appIdsBlock        AppId area of a queue record.
    /// @param numAppIds          Number of pairs the record declares.
    /// @returns 0 if `appIdsBlock` holds exactly `numAppIds` complete
    ///          entries, each with a non-empty appId and a non-null appKey,
    ///          a non-zero value otherwise.  On failure, `appIdKeyPairs`
    ///          holds the entries read before the offending one.
    static int loadAppInfos(mqbi::Storage::AppInfos* appIdKeyPairs,
                            const MemoryBlock&       appIdsBlock,
                            unsigned int             numAppIds);
};

}  // close package namespace
}  // close enterprise namespace

#endif
