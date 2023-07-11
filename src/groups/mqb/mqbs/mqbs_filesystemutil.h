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

// mqbs_filesystemutil.h                                              -*-C++-*-
#ifndef INCLUDED_MQBS_FILESYSTEMUTIL
#define INCLUDED_MQBS_FILESYSTEMUTIL

//@PURPOSE: Provide utilities for filesystem access.
//
//@CLASSES:
//  mqbs::FileSystemUtil: namespace for filesystem access methods.
//
//@DESCRIPTION: This component provides a utility struct,
// 'mqbs::FileSystemUtil', to work with a filesystem.

// MQB

// BDE
#include <ball_log.h>
#include <bsl_iosfwd.h>
#include <bsl_string.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

// FORWARD DECLARE
class MappedFileDescriptor;

// =====================
// struct FileSystemUtil
// =====================

/// This component provides utilities to work with a file system.
struct FileSystemUtil {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBS.FILESYSTEMUTIL");

  public:
    // CLASS METHODS

    /// Load into the specified `buffer` the name of the type of file system
    /// on which file with the specified `path` resides.  In case of error
    /// or an unknown file system type, `buffer` will be loaded with "OTHER"
    /// pattern.  Behavior is undefined unless `buffer` is non-null, and
    /// `path` is non-null and null-terminated.
    static void loadFileSystemName(bsl::string* buffer, const char* path);

    /// Load into the specified `available` and `total` buffers the
    /// available and total space respectively (in bytes) of the filesystem
    /// on which file with the specified `path` resides.  Return zero on
    /// success, and a non-zero value otherwise.  In case of failure, load
    /// human readable error information in the specified
    /// `errorDescription`.  Behavior is undefined unless `path` is non-null
    /// and null-terminated.
    static int loadFileSystemSpace(bsl::ostream&       errorDescription,
                                   bsls::Types::Int64* available,
                                   bsls::Types::Int64* total,
                                   const char*         path);

    /// Open the specified `filename` and map *at* *least* `fileSize` bytes
    /// of the file, and populate the specified `mfd` to represent the
    /// mapped file respecting the specified `readOnly` flag.  Return zero
    /// on success, non-zero otherwise with the specified `errorDescription`
    /// containing a detailed error.  Note that the mapped region may be
    /// greater than `fileSize` if `fileSize` is not a multiple of page
    /// size.
    static int open(MappedFileDescriptor* mfd,
                    const char*           filename,
                    bsls::Types::Uint64   fileSize,
                    bool                  readOnly,
                    bsl::ostream&         errorDescription,
                    bool                  prefaultPages = false);

    /// Unmap and close the file represented by the specified `mfd`.  Return
    /// zero on success, non-zero value otherwise.  The `mfd` is reset
    /// regardless of success or error.
    static int close(MappedFileDescriptor* mfd);

    /// Grow the file represented by the specified `fd` to the specified
    /// `fileSize`.  If the specified `reserveOnDisk` flag is true, also
    /// reserve the size on disk.  Return zero on success, non-zero value
    /// otherwise with the specified `errorDescription` containing a
    /// detailed error.  Note that if `reserveOnDisk` is true, this method
    /// will try to use syscall if supported by OS (syscall is more
    /// efficient), and if OS does not natively support this behavior, this
    /// routine will emulate it.
    static int grow(int                 fd,
                    bsls::Types::Uint64 fileSize,
                    bool                reserveOnDisk,
                    bsl::ostream&       errorDescription);

    /// Grow the file represented by the specified `mfd` to the size
    /// contained in `mfd`.  If the specified `reserveOnDisk` flag is true,
    /// also reserve the size on disk.  Return zero on success, non-zero
    /// value otherwise with the specified `errorDescription` containing a
    /// detailed error.  Note that if `reserveOnDisk` is true, this method
    /// will try to use syscall if supported by OS (syscall is more
    /// efficient), and if OS does not natively support this behavior, this
    /// routine will emulate it.
    static int grow(MappedFileDescriptor* mfd,
                    bool                  reserveOnDisk,
                    bsl::ostream&         errorDescription);

    /// Truncate the specified file `mfd` to the specified `size`.  Return
    /// zero on success, non-zero value otherwise with the specified
    /// `errorDescription` containing a detailed error.
    static int truncate(MappedFileDescriptor* mfd,
                        bsls::Types::Uint64   size,
                        bsl::ostream&         errorDescription);

    /// Move the specified file `oldAbsoluteFilename` at the specified
    /// `newLocation` directory.  Return zero on success, non-zero value
    /// otherwise.
    static int move(const bslstl::StringRef& oldAbsoluteFilename,
                    const bslstl::StringRef& newLocation);

    /// Reserve the specified `length` bytes on disk starting at the
    /// specified `offset` in the file represented by the specified `fd`.
    /// Return zero on success, non-zero value otherwise with the specified
    /// `errorDescription` containing a detailed error.  Note that no
    /// attempt is made to emulate fallocate behavior if OS/file-system
    /// doesn't support it.  Note that return value of `1` is reserved to
    /// indicate the absence of native support for fallocate() in the
    /// underlying OS/file-system.
    static int fallocate(int                 fd,
                         bsls::Types::Uint64 offset,
                         bsls::Types::Uint64 length,
                         bsl::ostream&       errorDescription);

    /// Execute the `madvise` system call using the specified `mapping`,
    /// `size`, and `advice`.
    static void madvise(void* mapping, bsls::Types::Uint64 size, int advice);

    /// Flush the memory-mapped `mapping` segment up to the specified
    /// `size`.  Return zero on success, a non-zero value otherwise with
    /// specified `errorDescription` containing a detailed error.
    static int flush(void*               mapping,
                     bsls::Types::Uint64 size,
                     bsl::ostream&       errorDescription);

    /// Indicate to the OS not to dump the specified `mapping` of the
    /// specified `size` to file.  Note that this method only has effect if
    /// on Linux and the `MADV_DONTDUMP` flag is defined.
    static void disableDump(void* mapping, bsls::Types::Uint64 size);
};

}  // close package namespace
}  // close enterprise namespace

#endif
