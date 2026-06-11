// Copyright 2017-2023 Bloomberg Finance L.P.
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

#ifndef INCLUDED_MQBS_FILESET
#define INCLUDED_MQBS_FILESET

//@PURPOSE: Provide a VST representing a set of BlazingMQ data store files.
//
//@CLASSES:
//  mqbs::FileSet: VST representing a set of BlazingMQ data store files
//
//@SEE ALSO: mqbs::FileStore
//
//@DESCRIPTION: This component provides a value-semantic type, 'mqbs::FileSet',
// representing a set of BlazingMQ data store files.

// MQB

#include <mqbs_mappedfiledescriptor.h>
#include <mqbu_storagekey.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbs {

// FORWARD DECLARE
class FileStore;

// ==============
// struct FileSet
// ==============

/// Value-semantic type representing a set of BlazingMQ data store files.
// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
struct FileSet BSLS_CPP11_FINAL {
  public:
    // PUBLIC DATA
    FileStore* d_store_p;

    mqbu::StorageKey d_dataFileKey;

    MappedFileDescriptor d_dataFile;

    MappedFileDescriptor d_journalFile;

    MappedFileDescriptor d_qlistFile;

    bsls::Types::Uint64 d_dataFilePosition;

    bsls::Types::Uint64 d_journalFilePosition;

    bsls::Types::Uint64 d_qlistFilePosition;

    bsl::string d_dataFileName;

    bsl::string d_journalFileName;

    bsl::string d_qlistFileName;

    bsls::Types::Uint64 d_outstandingBytesJournal;

    bsls::Types::Uint64 d_outstandingBytesData;

    bsls::Types::Uint64 d_outstandingBytesQlist;

    bool d_journalFileAvailable;

    bool d_fileSetRolloverPolicyAlarm;

    /// `true` if this FileSet is garbage-collected on rollover, `false` if
    /// GC is expected on the last alias destruction.
    bsls::AtomicBool d_inlineGc;

    /// The shared alias used to keep track of the current number of records
    /// that still reference this FileSet.  FileSet cannot be safely GCed
    /// unless all references are gone.
    bsl::shared_ptr<FileSet> d_aliasedChunk_sp;

    /// A weak_ptr pointing at the aliased chunk.
    /// Guaranteed to be never reset: safe to get `use_count`.
    bsl::weak_ptr<FileSet> d_aliasedChunk_wp;

    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED
    FileSet(const FileSet&) BSLS_CPP11_DELETED;
    FileSet& operator=(const FileSet&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FileSet, bslma::UsesBslmaAllocator)

    // CREATORS
    FileSet(FileStore* store, bslma::Allocator* allocator);

    // ACCESSORS
    /// Return the snapshot of the remaining number of references to this
    /// FileSet.
    long numReferences() const { return d_aliasedChunk_wp.use_count(); }
};
// NOLINTEND(cppcoreguidelines-special-member-functions)

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------
// class FileSet
// -------------

// CREATORS
inline FileSet::FileSet(FileStore* store, bslma::Allocator* allocator)
: d_store_p(store)
, d_dataFileKey()
, d_dataFile()
, d_journalFile()
, d_qlistFile()
, d_dataFilePosition(0)
, d_journalFilePosition(0)
, d_qlistFilePosition(0)
, d_dataFileName(allocator)
, d_journalFileName(allocator)
, d_qlistFileName(allocator)
, d_outstandingBytesJournal(0)
, d_outstandingBytesData(0)
, d_outstandingBytesQlist(0)
, d_journalFileAvailable(true)
, d_fileSetRolloverPolicyAlarm(false)
, d_inlineGc(false)
, d_aliasedChunk_sp()
, d_aliasedChunk_wp()
, d_allocator_p(allocator)
{
    BSLS_ASSERT(allocator);
}

}  // close package namespace
}  // close enterprise namespace

#endif
