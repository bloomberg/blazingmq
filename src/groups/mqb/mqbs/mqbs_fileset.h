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

// mqbs_fileset.h                                                     -*-C++-*-
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

    bsls::AtomicInt64 d_aliasedBlobBufferCount;
    // Number of payload blob buffers
    // referencing to the mapped data file
    // of this file set *PLUS* one.  See
    // notes in ctor of this component for
    // more details.

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
};

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
, d_aliasedBlobBufferCount(1)  // See note below explaining value of '1'
, d_allocator_p(allocator)
{
    BSLS_ASSERT(allocator);

    // The reason 'd_aliasedBlobBufferCount' is initialized with 1 instead of 0
    // is that we don't want the gc logic to kick in for the active file set
    // everytime its count goes to zero.  While there is a check in gc logic
    // which does not gc the file set if its active one, its still expensive to
    // invoke it (in the dispatcher thread) everytime count goes to 0.  In the
    // worst case, the count can go to 0 after everytime a message is deleted
    // from the store.  Here's how: assume that message flow is such that there
    // is only one outstanding message at any time, which is deleted after
    // consumer confirms it.
}

}  // close package namespace
}  // close enterprise namespace

#endif
