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

// mqbs_mappedfiledescriptor.h                                        -*-C++-*-
#ifndef INCLUDED_MQBS_MAPPEDFILEDESCRIPTOR
#define INCLUDED_MQBS_MAPPEDFILEDESCRIPTOR

//@PURPOSE: Provide a VST representing a memory mapped file.
//
//@CLASSES:
//  mqbs::MappedFileDescriptor: VST representing a memory mapped file.
//
//@SEE ALSO: mqbs::MappedFileIterator
//
//@DESCRIPTION: 'mqbs::MappedFileDescriptor' provides a value-semantic type
// representing a memory mapped file.

// MQB

#include <mqbs_memoryblock.h>

// BDE
#include <bslmf_isbitwiseequalitycomparable.h>
#include <bslmf_istriviallycopyable.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

// ==========================
// class MappedFileDescriptor
// ==========================

/// This component provides a VST representing a memory mapped file.
class MappedFileDescriptor {
  public:
    // CONSTANTS
    static const int                 k_INVALID_FILE_DESCRIPTOR = -1;
    static const bsls::Types::Uint64 k_INVALID_MAPPING_SIZE    = 0;
    static const char*               k_INVALID_MAPPING;

  private:
    // DATA
    int d_fd;  // File descriptor of the memory mapped
               // file

    bsls::Types::Uint64 d_fileSize;  // Size of the mapped file

    MemoryBlock d_block;  // Contiguous block of memory used for
                          // the mapping

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MappedFileDescriptor,
                                   bslmf::IsBitwiseEqualityComparable)
    BSLMF_NESTED_TRAIT_DECLARATION(MappedFileDescriptor,
                                   bsl::is_trivially_copyable)

    // CREATORS

    /// Create an invalid instance. `isValid` will return false.
    MappedFileDescriptor();

    // MANIPULATORS
    MappedFileDescriptor& setFd(int value);
    MappedFileDescriptor& setFileSize(bsls::Types::Uint64 value);
    MappedFileDescriptor& setBlock(const MemoryBlock& value);
    MappedFileDescriptor& setMapping(char* value);

    /// Set the corresponding field to the specified `value`.
    MappedFileDescriptor& setMappingSize(bsls::Types::Uint64 value);

    /// Reset this instance.  isValid() returns false after this.
    void reset();

    // ACCESSORS

    /// Return true if this instance is valid, false otherwise.
    bool isValid() const;

    int                 fd() const;
    bsls::Types::Uint64 fileSize() const;
    const MemoryBlock&  block() const;
    char*               mapping() const;

    /// Return the corresponding value.
    bsls::Types::Uint64 mappingSize() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class MappedFileDescriptor
// --------------------------

// CREATORS
inline MappedFileDescriptor::MappedFileDescriptor()
{
    reset();
}

// MANIPULATORS
inline MappedFileDescriptor& MappedFileDescriptor::setFd(int value)
{
    d_fd = value;
    return *this;
}

inline MappedFileDescriptor&
MappedFileDescriptor::setFileSize(bsls::Types::Uint64 value)
{
    d_fileSize = value;
    return *this;
}

inline MappedFileDescriptor&
MappedFileDescriptor::setBlock(const MemoryBlock& value)
{
    d_block = value;
    return *this;
}

inline MappedFileDescriptor& MappedFileDescriptor::setMapping(char* value)
{
    d_block.setBase(value);
    return *this;
}

inline MappedFileDescriptor&
MappedFileDescriptor::setMappingSize(bsls::Types::Uint64 value)
{
    d_block.setSize(value);
    return *this;
}

inline void MappedFileDescriptor::reset()
{
    d_fd       = k_INVALID_FILE_DESCRIPTOR;
    d_fileSize = 0;
    d_block.reset(0, k_INVALID_MAPPING_SIZE);
}

// ACCESSORS
inline int MappedFileDescriptor::fd() const
{
    return d_fd;
}

inline bsls::Types::Uint64 MappedFileDescriptor::fileSize() const
{
    return d_fileSize;
}

inline const MemoryBlock& MappedFileDescriptor::block() const
{
    return d_block;
}

inline char* MappedFileDescriptor::mapping() const
{
    return d_block.base();
}

inline bsls::Types::Uint64 MappedFileDescriptor::mappingSize() const
{
    return d_block.size();
}

inline bool MappedFileDescriptor::isValid() const
{
    if (k_INVALID_FILE_DESCRIPTOR == d_fd) {
        return false;  // RETURN
    }

    if (k_INVALID_MAPPING == d_block.base()) {
        return false;  // RETURN
    }

    if (k_INVALID_MAPPING_SIZE == d_block.size()) {
        return false;  // RETURN
    }

    return true;
}

}  // close package namespace
}  // close enterprise namespace

#endif
