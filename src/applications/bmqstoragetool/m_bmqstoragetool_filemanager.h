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

#ifndef INCLUDED_M_BMQSTORAGETOOL_FILEMANAGER_H
#define INCLUDED_M_BMQSTORAGETOOL_FILEMANAGER_H

//@PURPOSE: Provide access to journal, data and cluster state ledger (CSL) file
// iterators and method to build queue key/uri map.
//
//@CLASSES:
//  m_bmqstoragetool::FileManager                 : an interface class
//  m_bmqstoragetool::FileManagerImpl             : an implementation class
//  m_bmqstoragetool::FileManagerImpl::FileHandler: a RAII journal/data file
//  wrapper class
//  m_bmqstoragetool::FileManagerImpl::CslFileHandler: a RAII CSL
//  file wrapper class
//
//@DESCRIPTION:
//  'FileManager' provides an interface to access iterators and method to build
//  queue key/uri map. 'FileManagerImpl' works with real files. Provides access
//  to journal and
//    data files interators. Also retrieves a map of queue keys and names from
//    a CSL file.
//  'FileHandler'/'CslFileHandler' opens and closes the required files in RAII
//  technique.

// bmqstoragetool
#include <m_bmqstoragetool_queuemap.h>

// MQB
#include <mqbc_clusterstateledgerutil.h>
#include <mqbc_incoreclusterstateledgeriterator.h>
#include <mqbmock_logidgenerator.h>
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbsl_ledger.h>
#include <mqbsl_memorymappedondisklog.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

class FileManager {
  public:
    // CREATORS
    virtual ~FileManager();

    // MANIPULATORS
    /// Pointer to journal file iterator
    virtual mqbs::JournalFileIterator* journalFileIterator() = 0;
    /// Pointer to data file iterator
    virtual mqbs::DataFileIterator* dataFileIterator() = 0;
    /// Pointer to cluster state ledger file iterator. Note that
    /// `IncoreClusterStateLedgerIterator` type is used instead of
    /// interface `ClusterStateLedgerIterator` to get additional access to
    /// record offset.
    virtual mqbc::IncoreClusterStateLedgerIterator* cslFileIterator() = 0;

    // ACCESSORS
    /// Fill the specified `queueMap_p` with key/uri mapping from CSL file.
    virtual void fillQueueMapFromCslFile(QueueMap* queueMap_p) const = 0;
};

class FileManagerImpl : public FileManager {
  private:
    // PRIVATE TYPES
    template <typename ITER>
    class FileHandler {
      private:
        const bsl::string          d_path;
        ITER                       d_iter;
        mqbs::MappedFileDescriptor d_mfd;

      public:
        // CREATORS
        explicit FileHandler(const bsl::string& path,
                             bslma::Allocator*  allocator = 0);

        ~FileHandler();

        // ACCESSORS
        /// File path
        const bsl::string& path() const;

        // MANIPULATORS
        /// iterator resetter
        bool resetIterator(bsl::ostream& errorDescription);

        /// Mapped file iterator
        ITER* iterator();

        /// Mapped file descriptor
        mqbs::MappedFileDescriptor& mappedFileDescriptor();
    };

    class CslFileHandler {
      private:
        // PRIVATE DATA

        const bsl::string                                         d_path;
        bslma::ManagedPtr<mqbsl::Ledger>                          d_ledger_p;
        bslma::ManagedPtr<mqbc::IncoreClusterStateLedgerIterator> d_iter_p;
        bool              d_cslFromBegin;
        bslma::Allocator* d_allocator;

      public:
        // CREATORS
        explicit CslFileHandler(const bsl::string& path,
                                bool               cslFromBegin,
                                bslma::Allocator*  allocator = 0);

        ~CslFileHandler();

        // MANIPULATORS

        /// Reset iterator to the initial state.
        bool resetIterator(bsl::ostream& errorDescription);

        /// Return pointer to modifiable CSL file iterator.
        mqbc::IncoreClusterStateLedgerIterator* iterator();

        // ACCESSORS

        /// File path
        const bsl::string& path() const;

        /// Fill the specified `queueMap_p` with data from CSL file.
        void fillQueueMap(QueueMap* queueMap_p) const;
    };

    // PRIVATE DATA

    /// Handler of journal file
    FileHandler<mqbs::JournalFileIterator> d_journalFile;

    /// Handler of data file
    FileHandler<mqbs::DataFileIterator> d_dataFile;

    /// Handler of CSL file
    CslFileHandler d_cslFile;

  public:
    // CREATORS
    explicit FileManagerImpl(const bsl::string& journalFile,
                             const bsl::string& dataFile,
                             const bsl::string& cslFile,
                             bool               cslFromBegin,
                             bslma::Allocator*  allocator = 0);

    // MANIPULATORS
    mqbs::JournalFileIterator* journalFileIterator() BSLS_KEYWORD_OVERRIDE;
    mqbs::DataFileIterator*    dataFileIterator() BSLS_KEYWORD_OVERRIDE;
    mqbc::IncoreClusterStateLedgerIterator*
    cslFileIterator() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    /// Fill the specified `queueMap_p` with key/uri mapping from CSL file.
    void
    fillQueueMapFromCslFile(QueueMap* queueMap_p) const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ==================================
// class FileManagerImpl::FileHandler
// ==================================

template <typename ITER>
inline FileManagerImpl::FileHandler<ITER>::FileHandler(
    const bsl::string& path,
    bslma::Allocator*  allocator)
: d_path(path, allocator)
{
    // NOTHING
}

template <typename ITER>
inline FileManagerImpl::FileHandler<ITER>::~FileHandler()
{
    d_iter.clear();
    if (d_mfd.isValid()) {
        mqbs::FileSystemUtil::close(&d_mfd);
    }
}

template <typename ITER>
inline const bsl::string& FileManagerImpl::FileHandler<ITER>::path() const
{
    return d_path;
}

template <typename ITER>
inline ITER* FileManagerImpl::FileHandler<ITER>::iterator()
{
    return &d_iter;
}

template <typename ITER>
inline mqbs::MappedFileDescriptor&
FileManagerImpl::FileHandler<ITER>::mappedFileDescriptor()
{
    return d_mfd;
}

}  // close package namespace
}  // close enterprise namespace

#endif  // FILEMANAGER_H
