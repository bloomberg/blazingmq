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

// m_bmqstoragetool_filemanager.h -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_FILEMANAGER_H
#define INCLUDED_M_BMQSTORAGETOOL_FILEMANAGER_H

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
    // MANIPULATORS
    virtual mqbs::JournalFileIterator* journalFileIterator() = 0;
    virtual mqbs::DataFileIterator*    dataFileIterator()    = 0;

    virtual ~FileManager(){};
};

class FileManagerReal : public FileManager {
  private:
    // PRIVATE TYPES
    template <typename ITER>
    class FileHandler {
      public:
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
        bsl::string path() const;

        // MANIPULATORS
        /// iterator resetter
        bool resetIterator(bsl::ostream& errorDescription);

        /// Mapped file iterator
        ITER* iterator();

        /// Mapped file descriptor
        mqbs::MappedFileDescriptor& mappedFileDescriptor();
    };

    // PRIVATE DATA
    FileHandler<mqbs::JournalFileIterator> d_journalFile;
    // Handler of journal file
    FileHandler<mqbs::DataFileIterator> d_dataFile;
    // Handler of data file

  public:
    // CREATORS
    /// Default constructor
    explicit FileManagerReal(const bsl::string& journalFile,
                             const bsl::string& dataFile,
                             bslma::Allocator*  allocator = 0);

    // MANIPULATORS
    mqbs::JournalFileIterator* journalFileIterator() BSLS_KEYWORD_OVERRIDE;
    mqbs::DataFileIterator*    dataFileIterator() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC FUNCTIONS
    static QueueMap buildQueueMap(const bsl::string& cslFile,
                                  bslma::Allocator*  allocator);
    // Build queue map from csl file.
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

template <typename ITER>
inline FileManagerReal::FileHandler<ITER>::FileHandler(
    const bsl::string& path,
    bslma::Allocator*  allocator)
: d_path(path, allocator)
{
    // NOTHING
}

template <typename ITER>
inline FileManagerReal::FileHandler<ITER>::~FileHandler()
{
    d_iter.clear();
    if (d_mfd.isValid()) {
        mqbs::FileSystemUtil::close(&d_mfd);
    }
}

template <typename ITER>
inline bsl::string FileManagerReal::FileHandler<ITER>::path() const
{
    return d_path;
}

template <typename ITER>
inline ITER* FileManagerReal::FileHandler<ITER>::iterator()
{
    return &d_iter;
}

template <typename ITER>
inline mqbs::MappedFileDescriptor&
FileManagerReal::FileHandler<ITER>::mappedFileDescriptor()
{
    return d_mfd;
}

}  // close package namespace
}  // close enterprise namespace

#endif  // FILEMANAGER_H
