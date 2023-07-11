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

// mqbs_filestoreset.h                                                -*-C++-*-
#ifndef INCLUDED_MQBS_FILESTORESET
#define INCLUDED_MQBS_FILESTORESET

//@PURPOSE: Provide a VST to capture attributes of BlazingMQ files.
//
//@CLASSES:
//  mqbs::FileStoreSet: VST capturing attributes of BlazingMQ files.
//
//@DESCRIPTION: This component provides a VST class, 'mqbs::FileStoreSet', to
// capture the names of three files (data, journal, and qlist) that are part of
// a 'mqbs::FileStore' and some associated attributes.

// MQB

// BDE
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

namespace mqbs {

// ==================
// class FileStoreSet
// ==================

/// This class provides a VST to capture the names of three files (data,
/// journal and qlist) that are part of a `mqbs::FileStore`.
class FileStoreSet BSLS_CPP11_FINAL {
  private:
    // DATA
    bsl::string d_dataFile;

    bsls::Types::Uint64 d_dataFileSize;

    bsl::string d_journalFile;

    bsls::Types::Uint64 d_journalFileSize;

    bsl::string d_qlistFile;

    bsls::Types::Uint64 d_qlistFileSize;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FileStoreSet, bslma::UsesBslmaAllocator)

  public:
    // CREATORS
    explicit FileStoreSet(bslma::Allocator* basicAllocator = 0);

    FileStoreSet(const FileStoreSet& src,
                 bslma::Allocator*   basicAllocator = 0);

    // MANIPULATORS
    // FileStoreSet& operator=(const FileStoreSet&);
    FileStoreSet& setDataFile(const bsl::string& value);
    FileStoreSet& setDataFileSize(bsls::Types::Int64 value);
    FileStoreSet& setJournalFile(const bsl::string& value);
    FileStoreSet& setJournalFileSize(bsls::Types::Int64 value);
    FileStoreSet& setQlistFile(const bsl::string& value);

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to this object.
    FileStoreSet& setQlistFileSize(bsls::Types::Int64 value);

    /// Reset the members of this object.
    void reset();

    // ACCESSORS
    const bsl::string& dataFile() const;
    bsls::Types::Int64 dataFileSize() const;
    const bsl::string& journalFile() const;
    bsls::Types::Int64 journalFileSize() const;
    const bsl::string& qlistFile() const;

    /// Get the value of the corresponding attribute.
    bsls::Types::Int64 qlistFileSize() const;

    /// Write a human-readable string representation of this object to the
    /// specified output `stream`, and return a reference to `stream`.
    /// Optionally specify an initial indentation `level`, whose absolute
    /// value is incremented recursively for nested objects.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute value
    /// indicates the number of spaces per indentation level for this and
    /// all of its nested objects.  If `level` is negative, suppress
    /// indentation of the first line.  If `spacesPerLevel` is negative,
    /// format the entire output on one line, suppressing all but the
    /// initial indentation (as governed by `level`).
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const FileStoreSet& fileSet);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// class FileStoreSet
// ------------------

// CREATORS
inline FileStoreSet::FileStoreSet(bslma::Allocator* basicAllocator)
: d_dataFile(basicAllocator)
, d_dataFileSize(0)
, d_journalFile(basicAllocator)
, d_journalFileSize(0)
, d_qlistFile(basicAllocator)
, d_qlistFileSize(0)
{
    // NOTHING
}

inline FileStoreSet::FileStoreSet(const FileStoreSet& src,
                                  bslma::Allocator*   basicAllocator)
: d_dataFile(src.d_dataFile, basicAllocator)
, d_dataFileSize(src.d_dataFileSize)
, d_journalFile(src.d_journalFile, basicAllocator)
, d_journalFileSize(src.d_journalFileSize)
, d_qlistFile(src.d_qlistFile, basicAllocator)
, d_qlistFileSize(src.d_qlistFileSize)
{
    // NOTHING
}

// MANIPULATORS
inline FileStoreSet& FileStoreSet::setDataFile(const bsl::string& value)
{
    d_dataFile = value;
    return *this;
}

inline FileStoreSet& FileStoreSet::setDataFileSize(bsls::Types::Int64 value)
{
    d_dataFileSize = value;
    return *this;
}

inline FileStoreSet& FileStoreSet::setJournalFile(const bsl::string& value)
{
    d_journalFile = value;
    return *this;
}

inline FileStoreSet& FileStoreSet::setJournalFileSize(bsls::Types::Int64 value)
{
    d_journalFileSize = value;
    return *this;
}

inline FileStoreSet& FileStoreSet::setQlistFile(const bsl::string& value)
{
    d_qlistFile = value;
    return *this;
}

inline FileStoreSet& FileStoreSet::setQlistFileSize(bsls::Types::Int64 value)
{
    d_qlistFileSize = value;
    return *this;
}

// ACCESSORS
inline const bsl::string& FileStoreSet::dataFile() const
{
    return d_dataFile;
}

inline bsls::Types::Int64 FileStoreSet::dataFileSize() const
{
    return d_dataFileSize;
}

inline const bsl::string& FileStoreSet::journalFile() const
{
    return d_journalFile;
}

inline bsls::Types::Int64 FileStoreSet::journalFileSize() const
{
    return d_journalFileSize;
}

inline const bsl::string& FileStoreSet::qlistFile() const
{
    return d_qlistFile;
}

inline bsls::Types::Int64 FileStoreSet::qlistFileSize() const
{
    return d_qlistFileSize;
}

}  // close package namespace

// ------------------
// class FileStoreSet
// ------------------

// FREE OPERATORS
inline bsl::ostream& mqbs::operator<<(bsl::ostream&             stream,
                                      const mqbs::FileStoreSet& fileSet)
{
    return fileSet.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
