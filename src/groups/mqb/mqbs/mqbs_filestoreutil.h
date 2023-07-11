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

// mqbs_filestoreutil.h                                               -*-C++-*-
#ifndef INCLUDED_MQBS_FILESTOREUTIL
#define INCLUDED_MQBS_FILESTOREUTIL

//@PURPOSE: Provide utilities for BlazingMQ file store.
//
//@CLASSES:
//  mqbs::FileStoreUtil: Utilities for BlazingMQ file store.
//
//@SEE ALSO: mqbs::FileStore
//
//@DESCRIPTION: 'mqbs::FileStoreUtil' provides utilities to work with a
// BlazingMQ file store.

// MQB

#include <mqbcfg_messages.h>
#include <mqbs_datastore.h>
#include <mqbs_fileset.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreset.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <ball_log.h>
#include <bdlt_datetime.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace mqbs {

// FORWARD DECLARATIONS
class DataFileIterator;
class JournalFileIterator;
class MappedFileDescriptor;
class QlistFileIterator;

// ====================
// struct FileStoreUtil
// ====================

/// This component provides utilities to work with a BlazingMQ file store.
struct FileStoreUtil {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBS.FILESTOREUTIL");

  private:
    // PRIVATE TYPES

    /// timestamp (YYYYMMDD_HHMMSS) => FileStoreSet containing 3 files
    ///                                having same timestamp in their names
    /// `FileSetMap` is an alias for fileStoreSets in sorted order of
    /// lexicographically increasing timestamps.  Used when organizing a
    /// list of full file paths into the associated fileStoreSets.  Note
    /// that a `bsl::map` will keep the FileStoreSet entries in sorted order
    /// of lexicographically increasing timestamps (whereas a
    /// `bsl::unordered_map` would not)
    typedef bsl::map<bsl::string, FileStoreSet> FileSetMap;

  public:
    typedef bsl::shared_ptr<FileSet> FileSetSp;

  private:
    // PRIVATE CLASS METHODS

    /// Load into the specified `fileSetMap` the sets of (data, journal,
    /// qlist) files extracted from the specified `files`.  If the
    /// optionally specified `withSize` is true, additionally populate the
    /// size of each file in the output file sets.
    static int
    findFileStoreSetsFromPaths(FileSetMap*                     fileSetMap,
                               const bsl::vector<bsl::string>& files,
                               bool                            withSize);

  public:
    // CLASS METHODS

    /// Unmap and close the files associated with a single partition (not
    /// enforced) and represented by the specified `dataFileMfd`,
    /// `journalFileMfd`, and `qlistFileMfd`.  Return zero on success,
    /// non-zero value otherwise.  The `dataFileMfd`, `journalFileMfd`, and
    /// `qlistFileMfd` are reset regardless of success or error.
    static int closePartitionSet(MappedFileDescriptor* dataFileMfd,
                                 MappedFileDescriptor* journalFileMfd,
                                 MappedFileDescriptor* qlistFileMfd);

    static void createDataFileName(bsl::string*             filename,
                                   const bslstl::StringRef& basePath,
                                   int                      partitionId,
                                   const bdlt::Datetime&    datetime);
    static void createJournalFileName(bsl::string*             filename,
                                      const bslstl::StringRef& basePath,
                                      int                      partitionId,
                                      const bdlt::Datetime&    datetime);

    /// Populate the specified `filename` with the corresponding file name
    /// (data, journal and qlist respectively) located at the specified
    /// `basePath` location, and having the specified `partitionId` and
    /// `datetime` attributes.
    static void createQlistFileName(bsl::string*             filename,
                                    const bslstl::StringRef& basePath,
                                    int                      partitionId,
                                    const bdlt::Datetime&    datetime);

    static bool hasDataFileExtension(const bsl::string& filename);
    static bool hasJournalFileExtension(const bsl::string& filename);

    /// Return true if the specified `filename` ends with the corresponding
    /// (data, journal or qlist respectively) extension.
    static bool hasQlistFileExtension(const bsl::string& filename);

    /// Populate the specified `pattern` with a string pattern which can be
    /// used to search BlazingMQ files belonging to the specified
    /// `partitionId` located at the specified `basePath` location.  Return
    /// zero on success, non-zero value otherwise.
    static int createFilePattern(bsl::string*             pattern,
                                 const bslstl::StringRef& basePath,
                                 int                      partitionId);

    /// Populate the specified `pattern` with a string pattern which can be
    /// used to search BlazingMQ files located at the specified `basePath`
    /// location.  Return zero on success, non-zero value otherwise.
    static int createFilePattern(bsl::string*             pattern,
                                 const bslstl::StringRef& basePath);

    /// Create all the relevant files names, open them for writing and
    /// populate the specified `fileSetSp` for the specified `partitionId`
    /// with relevant information, using the specified `fileStore` and
    /// `dataStoreConfig`.  The specified `partitionDesc` is used for
    /// logging purposes.  The specified `needQList` determines whether to
    /// create and open the QList file.  Use the specified `allocator` for
    /// memory allocations.  Return zero on success, non-zero value
    /// otherwise along with populating the specified `errorDescription`
    /// with a brief reason for logging purposes.  Note that all current
    /// values in the `fileSetSp` (if any) are overwritten, and no attempt
    /// is made to close any valid files which `fileSetSp` may be holding.
    /// Note that BlazingMQ header and file-specific header are written to
    /// the newly created files.
    static int create(bsl::ostream&            errorDescription,
                      FileSetSp*               fileSetSp,
                      FileStore*               fileStore,
                      int                      partitionId,
                      const DataStoreConfig&   dataStoreConfig,
                      const bslstl::StringRef& partitionDesc,
                      bool                     needQList,
                      bslma::Allocator*        allocator);

    /// Populate the specified `timestamp` with the `YYYYMMDD_HHMMSS`
    /// pattern extracted from the specified BlazingMQ `filename`.  Return
    /// zero on success, non-zero value otherwise.
    static int extractTimestamp(bsl::string*       timestamp,
                                const bsl::string& filename);

    /// Load into the specified `fileStoreSet` the specified active
    /// (current) `fileSet`.  Only load the QList file if the specified
    /// `needQList` is true.
    static void loadCurrentFiles(FileStoreSet*  fileStoreSet,
                                 const FileSet& fileSet,
                                 bool           needQList = true);

    /// Populate the specified `fileSets` with the list of sets of BlazingMQ
    /// files (data, journal and qlist) located at the specified `basePath`
    /// location and belonging to the specified `partitionId`.  If the
    /// optionally specified `withSize` flag is true, additionally populate
    /// in each of the `fileSets` the corresponding size of each file.
    /// Return zero on success, non-zero value otherwise.  Note that the
    /// file sets are sorted from oldest to newest in the resultant
    /// `fileSets`, with oldest being the first element and newest being the
    /// last element.  Also note that the timestamp part (YYYYMMDD_HHMMSS)
    /// in the file names is used to sort the sets. Note, this function will
    /// check for QList files only if optionally specified `needQList` is
    /// true.
    static int findFileSets(bsl::vector<FileStoreSet>* fileSets,
                            const bslstl::StringRef&   basePath,
                            int                        partitionId,
                            bool                       withSize  = false,
                            bool                       needQList = true);

    /// Delete the archived files located at the specified `archiveLocation`
    /// belonging to the specified `partitionId` of the specified `cluster`
    /// and keep at most a maximum of the specified `maxArchivedFileSets`.
    /// Behavior is undefined unless `archiveLocation` exists and is a
    /// directory.  Note that `cluster` is used for logging purposes only.
    static void deleteArchiveFiles(int                partitionId,
                                   const bsl::string& archiveLocation,
                                   int                maxArchivedFileSets,
                                   const bsl::string& cluster);

    /// Delete the archived files belonging to *all* partitions in the
    /// specified `partitionCfg` of the specified `cluster`.  Note that
    /// `cluster` is used for logging purposes only.
    static void deleteArchiveFiles(const mqbcfg::PartitionConfig& partitionCfg,
                                   const bsl::string&             cluster);

    /// Populate the optionally specified `journalFd`, `dataFd` and
    /// `qlistFd` file descriptors with the corresponding journal, data and
    /// qlist file representations respectively **if specified**, and open
    /// the **specified** files in the specified `fileSet` in the read-only
    /// mode.  At least one of `journalFd`, `dataFd` or `qlistFd` must be
    /// specified.  Return zero on success, non-zero value otherwise along
    /// with populating the specified `errorDescription` with a brief reason
    /// for logging purposes.  Note that in case of errors, this method
    /// closes any files it opened.
    static int openFileSetReadMode(bsl::ostream&         errorDescription,
                                   const FileStoreSet&   fileSet,
                                   MappedFileDescriptor* journalFd = 0,
                                   MappedFileDescriptor* dataFd    = 0,
                                   MappedFileDescriptor* qlistFd   = 0);

    /// Populate the optionally specified `journalFd`, `dataFd` and
    /// `qlistFd` file descriptors with the corresponding journal, data and
    /// qlist file representations respectively **if specified**, and open
    /// the **specified** files in the specified `fileSet` in the write
    /// mode.  At least one of `journalFd`, `dataFd` or `qlistFd` must be
    /// specified.  Return zero on success, non-zero value otherwise along
    /// with populating the specified `errorDescription` with a brief reason
    /// for logging purposes.  If the specified `preallocate` flag is true,
    /// reserve the space for the files on disk.  If the specified
    /// `deleteOnFailure` flag is true, delete the files on disk on failure.
    /// Note that in case of errors, this method closes any files it opened.
    static int openFileSetWriteMode(bsl::ostream&         errorDescription,
                                    const FileStoreSet&   fileSet,
                                    bool                  preallocate,
                                    bool                  deleteOnFailure,
                                    MappedFileDescriptor* journalFd = 0,
                                    MappedFileDescriptor* dataFd    = 0,
                                    MappedFileDescriptor* qlistFd   = 0,
                                    bool prefaultPages              = false);

    /// Validate the journal, qlist and data files represented by the
    /// specified `journalFd`, `qlistFd` and `dataFd` respectively.
    /// **Skip** validation of any file descriptor which is **invalid**.
    /// Return 0 on success, non-zero value otherwise.
    ///
    /// TBD: explicitly list the fields/attributes which are checked as part
    ///     of validation.
    static int validateFileSet(const MappedFileDescriptor& journalFd,
                               const MappedFileDescriptor& dataFd,
                               const MappedFileDescriptor& qlistFd);

    /// Retrieve the appropriate file set belonging to the specified
    /// `partitionId` from the specified `config` location from which
    /// recovery needs to be performed, checking a maximum of the specified
    /// `numSetsToCheck` file sets if multiple sets are present, open the
    /// corresponding journal, data and qlist files (if **specified**) in
    /// the specified `journalFd`, and the optionally specified `dataFd` and
    /// `qlistFd` respectively, and update the specified `recoveryFileSet`
    /// with the retrieved set, and archive the remaining sets to the
    /// specified `config` archiveLocation.  If the specified `readOnly` is
    /// true, open the files in read-only mode. The behavior also depends
    /// on the specified `isFSMWorkflow` flag.  Return 0 on success, non
    /// zero value otherwise along with populating the specified
    /// `errorDescription` with a brief reason for logging purposes.  Note
    /// that a return value of `1` is special and indicates that no file
    /// sets were present at `config` location.  Also note that in case of
    /// error, this method closes any files it opened.
    static int openRecoveryFileSet(bsl::ostream&         errorDescription,
                                   MappedFileDescriptor* journalFd,
                                   FileStoreSet*         recoveryFileSet,
                                   int                   partitionId,
                                   int                   numSetsToCheck,
                                   const mqbs::DataStoreConfig& config,
                                   bool                         readOnly,
                                   bool                         isFSMWorkflow,
                                   MappedFileDescriptor*        dataFd  = 0,
                                   MappedFileDescriptor*        qlistFd = 0);

    /// Load into the specified `jit`, `dit` and `qit` the iterators
    /// corresponding to the already opened specified `journalFd`, `dataFd`
    /// and `qlistFd` files respectively, with the names of the three files
    /// captured in the specified `fileSet`.  Return 0 on success, non-zero
    /// value otherwise along with populating the specified
    /// `errorDescription` with a brief reason for logging purposes.
    /// Behavior is undefined unless `journalFd`, `qlistFd` and `dataFd`
    /// represent opened files. The optionally specified `needData` and
    /// `needQList` values make sure to not do the work to load data and
    /// QList iterators respectively when they are not needed.
    static int loadIterators(bsl::ostream&               errorDescription,
                             JournalFileIterator*        jit,
                             DataFileIterator*           dit,
                             QlistFileIterator*          qit,
                             const MappedFileDescriptor& journalFd,
                             const MappedFileDescriptor& dataFd,
                             const MappedFileDescriptor& qlistFd,
                             const FileStoreSet&         fileSet,
                             bool                        needQList = true,
                             bool                        needData  = true);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

}  // close package namespace

}  // close enterprise namespace

#endif
