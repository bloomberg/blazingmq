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

// mqbs_filesystemutil.cpp                                            -*-C++-*-
#include <mqbs_filesystemutil.h>

#include <mqbscm_version.h>
// IMPLEMENTATION NOTES

/// Reason for not using posix_fallocate()
///--------------------------------------
//
// 1) If native fallocate support is not available, posix_fallocate() emulates
//    fallocate behavior by writing one char every 32KB in the file, which can
//    be slow for files with large sizes.  Not to mention, entire file gets
//    paged in right away.
//
// 2) Solaris 10 does not support posix_fallocate().
//
//
/// Native fallocate support on different OSes/file-systems
///-------------------------------------------------------
//
// 1) Linux:
//   Supported since version 2.6.23 on ext4, xfs, btrfs etc file systems, but
//   not on ext3.
//
// 2) Solaris:
//   No-op on solaris 10: fcntl(..., F_ALLOCSP, ...) returns EINVAL.
//   Supported on solaris 11 on UFS. There is no concept of fallocate on ZFS
//   since its copy-on-write.
//
// 3) OSX:
//   Supported: fcntl(..., F_PREALLOCATE, ...)
//
// 4) AIX:
//   No support
//
//
/// Notes regarding madvise() on Linux
///----------------------------------
//
// 1) The two most appropriate madvise-flags for BlazingMQ broker are
//    MADV_SEQUENTIAL and MADV_WILLNEED.
//
// 2) Upon seeing the MADV_SEQUENTIAL hint, kernel could perform aggressive
//    read-ahead of the files, which could be good for the broker.  However, it
//    also means that kernel could aggressively free the written pages.  This
//    is not ideal for the broker, because although writes are sequential for
//    it, reads can be random.
//
// 3) When kernel sees MADV_WILLNEED advise, it asynchronously issues a
//    read-ahead of all (most?) pages in the range specified in the madvise()
//    call.  Iterating over this range takes time, especially for large files
//    used by the broker (up to 5 seconds or more).  Since a partition is
//    created in the storage-dispatcher thread, it could block the thread for
//    that much time, which is not good for the latency.
//
// 4) Another subtle issue with madvise(MADV_WILLNEED) is that during the
//    iteration over the specified memory range, Linux kernel takes a read-lock
//    over the `mmap_sem` lock.  This means that while this operation is in
//    progress, any other thread in the broker invoking a syscall which takes a
//    write-lock over this lock will block.  Two such syscalls are mmap() and
//    munmap() (there could be others).  It was noticed on dev that one of the
//    IO threads of the brokers was stuck like this, because it was trying to
//    munmap() '/etc/resolv.conf' file as part of hostname resolution step
//    performed by our network library.  It was noticed that IO thread was
//    stuck for more than 10 seconds because other thread was in the middle of
//    madvise(MADV_WILLNEED).
//
//
/// Micro-benchmarking numbers on Linux
///-----------------------------------
// Micro-benchmark was performed on Linux to see the effect of madvise() and
// some mmap()-related flags.
//
// Environment:
//   - Broker: 64bit, debug build.
//   - OS    : RHEL 7.3 (Maipo)
//   - FS    : XFS (Mount options: rw,relatime,attr2,inode64,sunit=512,
//                                 swidth=512,noquota 0 0)
//
// Program (pseudocode):
//   - Create a 6GB file on the filesystem.
//   - Grow the file (ftruncate or fallocate)
//   - mmap() the file (optionally pass 'MAP_POPULATE' flag)
//   - Optionally invoke madvise() with either MADV_SEQUENTIAL or MADV_WILLNEED
//   - Sleep a few seconds to let madvise() take effect etc.
//   - In a timed loop, memcpy() a buffer of 26 bytes to the mapped file.
//
//..
// +----------------+--------------+--------------+------------------+
// |     Grow       | MAP_POPULATE |    madvise   | Time (millisec)  |
// +================+==============+==============+==================+
// |   fallocate()  |      N       |    WILLNEED  |       3400       |
// +----------------+--------------+--------------+------------------+
// |   ftruncate()  |      N       |    WILLNEED  |       3800       |
// +----------------+--------------+--------------+------------------+
// |   fallocate()  |      Y       |    WILLNEED  |       3900       |
// +----------------+--------------+--------------+------------------+
// |   fallocate()  |      Y       |   SEQUENTIAL |       3900       |
// +----------------+--------------+--------------+------------------+
// |   fallocate()  |      Y       |       -      |       4000       |
// +----------------+--------------+--------------+------------------+
// |   ftruncate()  |      Y       |    WILLNEED  |       4250       |
// +----------------+--------------+--------------+------------------+
// |   ftruncate()  |      Y       |   SEQUENTIAL |       4400       |
// +----------------+--------------+--------------+------------------+
// |   fallocate()  |      N       |   SEQUENTIAL |       5000       |
// +----------------+--------------+--------------+------------------+
// |   fallocate()  |      N       |       -      |       5100       |
// +----------------+--------------+--------------+------------------+
// |   ftruncate()  |      Y       |       -      | 4200-5400(varies)|
// +----------------+--------------+--------------+------------------+
// |   ftruncate()  |      N       |   SEQUENTIAL |       5350       |
// +----------------+--------------+--------------+------------------+
// |   ftruncate()  |      N       |       -      |       5500       |
// +----------------+--------------+--------------+------------------+
//..
//
// As per above numbers, it seems that most efficient combination is
// 'fallocate() + madvise(WILLNEED)', but given the issues with madvise()
// discussed in the previous section, we decide to go with the 'fallocate() +
// MAP_POPULATE' option.
//
// If at any time, it is desired to also use madvise(WILLNEED), one way to
// achieve that would be to invoke it asynchronously for small chunks of the
// mapped file, so as not to block the storage-dispatcher thread, as well as to
// minimize the amount of time 'mmap_sem' lock is held by the kernel.
//
// Note that a local broker setup was also benchmarked for latency with various
// combinations listed in the table above, but numbers varied too much and
// there was no definite pattern.

// MQB
#include <mqbs_mappedfiledescriptor.h>

// BDE
#include <bdlb_string.h>
#include <bdls_filesystemutil.h>
#include <bdls_pathutil.h>
#include <bsl_ostream.h>
#include <bsls_assert.h>
#include <bsls_platform.h>

// SYS
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(BSLS_PLATFORM_OS_LINUX)
#include <sys/statfs.h>
#include <sys/vfs.h>  // for statfs
#endif

#ifdef BSLS_PLATFORM_OS_DARWIN
#include <sys/mount.h>
#include <sys/param.h>  // for statfs
#endif

// GCC on Solaris 10 cannot find madvise declaration. Providing one explicitly.
#if defined(BSLS_PLATFORM_OS_SOLARIS) && defined(BSLS_PLATFORM_CMP_GNU)
extern "C" int madvise(caddr_t, size_t, int);
#endif

namespace BloombergLP {
namespace mqbs {

namespace {

const char k_OTHER_FS_NAME[] = "OTHER";

#ifdef BSLS_PLATFORM_OS_LINUX

// Following magic constants have been copied from <linux/magic.h> which is not
// available on all of our linux environments at build time.

const long k_MAGIC_EXT   = 0xEF53;  // EXT2, EXT3 & EXT4 use same magic value
const long k_MAGIC_XFS   = 0x58465342;
const long k_MAGIC_NFS   = 0x6969;
const long k_MAGIC_TMPFS = 0x01021994;
const long k_MAGIC_RAMFS = 0x858458F6;
const long k_MAGIC_BTRFS = 0x9123683E;

void loadNameFromFsType(bsl::string* buffer, long ftype)
{
    switch (ftype) {
    case k_MAGIC_EXT: buffer->assign("EXT"); return;  // RETURN

    case k_MAGIC_XFS: buffer->assign("XFS"); return;  // RETURN

    case k_MAGIC_NFS: buffer->assign("NFS"); return;  // RETURN

    case k_MAGIC_TMPFS: buffer->assign("TMPFS"); return;  // RETURN

    case k_MAGIC_RAMFS: buffer->assign("RAMFS"); return;  // RETURN

    case k_MAGIC_BTRFS: buffer->assign("BTRFS"); return;  // RETURN

    default: {
        // Include the hex numeric value of 'ftype', which can be looked up in
        // <linux/magic.h> header during troubleshooting.
        bsl::ostringstream os;
        os << "OTHER: 0x" << bsl::hex << ftype;
        buffer->assign(os.str());
        return;  // RETURN
    }
    };
}

#endif

}  // close unnamed namespace

// ---------------------
// struct FileSystemUtil
// ---------------------

void FileSystemUtil::loadFileSystemName(bsl::string* buffer, const char* path)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(path);
    BSLS_ASSERT_SAFE(buffer);

#if defined(BSLS_PLATFORM_OS_LINUX)

    struct ::statfs buf;
    int             rc = ::statfs(path, &buf);
    if (0 != rc) {
        BALL_LOG_WARN << "statfs() failed for [" << path << "], rc: " << rc
                      << ", errno: " << errno << " [" << bsl::strerror(errno)
                      << "].";
        buffer->assign(k_OTHER_FS_NAME);
        return;  // RETURN
    }

    loadNameFromFsType(buffer, buf.f_type);
    return;  // RETURN

#elif defined(BSLS_PLATFORM_OS_SOLARIS) || defined(BSLS_PLATFORM_OS_AIX)

    struct ::statvfs buf;
    int              rc = ::statvfs(path, &buf);
    if (0 != rc) {
        BALL_LOG_WARN << "statvfs() failed for [" << path << "], rc: " << rc
                      << ", errno: " << errno << " [" << bsl::strerror(errno)
                      << "].";
        buffer->assign(k_OTHER_FS_NAME);
        return;  // RETURN
    }

    buffer->assign(buf.f_basetype);
    bdlb::String::toUpper(buffer);
    return;  // RETURN

#elif defined(BSLS_PLATFORM_OS_DARWIN)

    struct ::statfs buf;
    int             rc = ::statfs(path, &buf);
    if (0 != rc) {
        BALL_LOG_WARN << "statfs() failed for [" << path << "], rc: " << rc
                      << ", errno: " << errno << " [" << bsl::strerror(errno)
                      << "].";
        buffer->assign(k_OTHER_FS_NAME);
        return;  // RETURN
    }

    buffer->assign(buf.f_fstypename);
    bdlb::String::toUpper(buffer);
    return;  // RETURN

#else

    buffer->assign(k_OTHER_FS_NAME);
    return;  // RETURN

#endif
}

int FileSystemUtil::loadFileSystemSpace(bsl::ostream&       errorDescription,
                                        bsls::Types::Int64* available,
                                        bsls::Types::Int64* total,
                                        const char*         path)
{
    // PRECONDTIONS
    BSLS_ASSERT_SAFE(path);
    BSLS_ASSERT_SAFE(available);
    BSLS_ASSERT_SAFE(total);

    struct ::statvfs buf;
    int              rc = ::statvfs(path, &buf);
    if (0 != rc) {
        errorDescription << "statvfs() failure for path [" << path
                         << "], errno: " << errno << " ["
                         << bsl::strerror(errno) << "]";
        return rc;  // RETURN
    }

    *available = static_cast<bsls::Types::Int64>(buf.f_bavail) *
                 static_cast<bsls::Types::Int64>(buf.f_frsize);

    *total = static_cast<bsls::Types::Int64>(buf.f_blocks) *
             static_cast<bsls::Types::Int64>(buf.f_frsize);
    return 0;
}

int FileSystemUtil::open(MappedFileDescriptor* mfd,
                         const char*           filename,
                         bsls::Types::Uint64   fileSize,
                         bool                  readOnly,
                         bsl::ostream&         errorDescription,
                         bool                  prefaultPages)
{
    enum { rc_SUCCESS = 0, rc_OPEN_FAILURE = -1, rc_MMAP_FAILURE = -2 };

    int fd    = -1;
    int oflag = readOnly ? O_RDONLY : (O_RDWR | O_CREAT);
    if (0 > (fd = ::open(filename,
                         oflag,
                         S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))) {
        errorDescription << "open() failure for file [" << filename
                         << "], errno: " << errno << " ["
                         << bsl::strerror(errno) << "]";
        return rc_OPEN_FAILURE;  // RETURN
    }

    bsls::Types::Uint64 mappingSize = fileSize;
    size_t              pageSize    = ::sysconf(_SC_PAGESIZE);
    if (0 != fileSize % pageSize) {
        mappingSize = (fileSize / pageSize + 1) * pageSize;
    }

    // mmap the file
    int protFlag = readOnly ? PROT_READ : (PROT_READ | PROT_WRITE);

    int mmapFlag = MAP_SHARED;  // common to all platforms

    if (prefaultPages) {
        // The linux-specific 'MAP_POPULATE' flag seems to make a difference of
        // *at least* 2x in the benchmarks.  Here's what man page for mmap says
        // about 'MAP_POPULATE' flag:
        //
        // MAP_POPULATE (since Linux 2.5.46)
        //     Populate (prefault) page tables for a mapping.  For a file
        //     mapping, this causes read-ahead on the file.  Later accesses to
        //     the mapping will not be blocked by page faults.
#if defined(BSLS_PLATFORM_OS_LINUX)
        mmapFlag |= MAP_POPULATE;
#else
        BALL_LOG_WARN << "Prefaulting pages not supported on this platform.";
#endif
    }

    char* base;
    base = static_cast<char*>(
        ::mmap(0, mappingSize, protFlag, mmapFlag, fd, 0));

    if (MAP_FAILED == base) {
        errorDescription << "mmap() failure for file [" << filename
                         << "], fd [" << fd << "], errno: " << errno << " ["
                         << bsl::strerror(errno) << "]";
        ::close(fd);
        if (!readOnly) {
            bdls::FilesystemUtil::remove(filename);
        }
        return rc_MMAP_FAILURE;  // RETURN
    }

    mfd->setFd(fd);
    mfd->setFileSize(fileSize);
    mfd->setMapping(base);
    mfd->setMappingSize(mappingSize);

    return 0;
}

int FileSystemUtil::close(MappedFileDescriptor* mfd)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(mfd);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS      = 0,
        rc_INVALID_MFD  = -1,
        rc_ERROR_MUNMAP = -2,
        rc_ERROR_CLOSE  = -3
    };

    int returnRC = rc_SUCCESS;

    // If any of the properties of 'mfd' is invalid, we assume entire 'mfd' is
    // invalid and return.  We could assert.
    if (!mfd->isValid()) {
        return rc_INVALID_MFD;  // RETURN
    }

    // unmap, close and reset the 'mfd'
    int rc = ::munmap(mfd->mapping(), mfd->mappingSize());
    if (rc != 0) {
        BALL_LOG_ERROR << "'munmap('" << mfd->mapping() << ", "
                       << mfd->mappingSize() << ")' failure for file fd ["
                       << mfd->fd() << "], rc: " << rc << ", errno: " << errno
                       << " [" << bsl::strerror(errno) << "]";

        // We may still be able to close the file, so don't return here but do
        // note the occurence of this error in the return code.
        returnRC = 10 * returnRC + rc_ERROR_MUNMAP;
    }

    rc = ::close(mfd->fd());
    if (rc != 0) {
        BALL_LOG_ERROR << "'close('" << mfd->fd() << ")' failure for file fd ["
                       << mfd->fd() << "], rc: " << rc << ", errno: " << errno
                       << " [" << bsl::strerror(errno) << "]";
        returnRC = 10 * returnRC + rc_ERROR_CLOSE;
    }

    mfd->reset();

    return returnRC;
}

int FileSystemUtil::grow(int                 fd,
                         bsls::Types::Uint64 fileSize,
                         bool                reserveOnDisk,
                         bsl::ostream&       errorDescription)
{
    enum {
        rc_SUCCESS                     = 0,
        rc_FALLOCATE_EMULATION_FAILURE = -1,
        rc_FTRUNCATE_FAILURE           = -2
    };

    int rc = 0;
    if (reserveOnDisk) {
        if (0 != (rc = FileSystemUtil::fallocate(fd,
                                                 0,
                                                 fileSize,
                                                 errorDescription))) {
            // fallocate failed, emulate fallocate behavior
            rc = bdls::FilesystemUtil::growFile(fd, fileSize, true);
            if (0 != rc) {
                errorDescription << ". Failed to emulate fallocate behavior, "
                                 << "rc: " << rc;
                rc = rc_FALLOCATE_EMULATION_FAILURE;
            }
        }

        return rc;  // RETURN
    }

    // No reserve-on-disk requested. Just change the file size
    rc = ::ftruncate(fd, fileSize);
    if (0 != rc) {
        errorDescription << "ftruncate() failed for fd [" << fd
                         << "], rc: " << rc << ", errno: " << errno << " ["
                         << bsl::strerror(errno) << "]";
        return rc_FTRUNCATE_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

int FileSystemUtil::grow(MappedFileDescriptor* mfd,
                         bool                  reserveOnDisk,
                         bsl::ostream&         errorDescription)
{
    enum RcEnum { rc_SUCCESS = 0, rc_INVALID_MFD = -1, rc_GROW_ERROR = -2 };

    if (!mfd->isValid()) {
        return rc_INVALID_MFD;  // RETURN
    }

    int rc = grow(mfd->fd(), mfd->fileSize(), reserveOnDisk, errorDescription);
    if (rc != 0) {
        return (rc * 10) + rc_GROW_ERROR;  // RETURN
    }

    return rc_SUCCESS;
}

int FileSystemUtil::truncate(MappedFileDescriptor* mfd,
                             bsls::Types::Uint64   size,
                             bsl::ostream&         errorDescription)
{
    enum { rc_SUCCESS = 0, rc_FAILURE = -1 };

    int rc = ::ftruncate(mfd->fd(), size);
    if (0 != rc) {
        errorDescription << "ftruncate() failed for file fd [" << mfd->fd()
                         << "], rc: " << rc << ", errno: " << errno << " ["
                         << bsl::strerror(errno) << "]";
        return rc_FAILURE;  // RETURN
    }

    mfd->setFileSize(size);

    return rc_SUCCESS;
}

int FileSystemUtil::move(const bslstl::StringRef& oldAbsoluteFilename,
                         const bslstl::StringRef& newLocation)
{
    enum {
        rc_SUCCESS            = 0,
        rc_EMPTY_NEW_LOCATION = -1,
        rc_NO_LEAF            = -2,
        rc_APPEND_FAILURE     = -3,
        rc_MOVE_FAILURE       = -4
    };

    if (newLocation.isEmpty()) {
        return rc_EMPTY_NEW_LOCATION;  // RETURN
    }

    bsl::string oldFileLeafName;  // not containing path
    int rc = bdls::PathUtil::getLeaf(&oldFileLeafName, oldAbsoluteFilename);
    if (0 != rc) {
        return rc * 10 + rc_NO_LEAF;  // RETURN
    }

    bsl::string newAbsoluteFileName(newLocation);
    rc = bdls::PathUtil::appendIfValid(&newAbsoluteFileName, oldFileLeafName);
    if (0 != rc) {
        return rc * 10 + rc_APPEND_FAILURE;  // RETURN
    }

    rc = bdls::FilesystemUtil::move(oldAbsoluteFilename, newAbsoluteFileName);
    if (0 != rc) {
        return rc * 10 + rc_MOVE_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

int FileSystemUtil::fallocate(int                 fd,
                              bsls::Types::Uint64 offset,
                              bsls::Types::Uint64 length,
                              bsl::ostream&       errorDescription)
{
    enum {
        rc_UNSUPPORTED     = 1,
        rc_SUCCESS         = 0,
        rc_SYSCALL_FAILURE = -1,
        rc_MISC_FAILURE    = -2
    };

#if defined(BSLS_PLATFORM_OS_LINUX)

    int rc = ::fallocate(fd, 0 /* mode */, offset, length);

    if (0 != rc) {
        errorDescription << "Failed to fallocate file with fd [" << fd
                         << "], rc: " << rc << ", errno: " << errno << "["
                         << bsl::strerror(errno) << "]";
        return rc_SYSCALL_FAILURE;  // RETURN
    }

    return rc_SUCCESS;

#elif defined(BSLS_PLATFORM_OS_SOLARIS) || defined(BSLS_PLATFORM_OS_SUNOS)
    // This will succeed only on solaris 11 with ufs

    struct flock64 fl;
    fl.l_whence = SEEK_SET;  // from start of file
    fl.l_start  = offset;    // offset from 'l_whence' (offset from offset)
    fl.l_len    = length;
    fl.l_type   = F_WRLCK;  // TBD: Needed?

    int rc = ::fcntl(fd, F_ALLOCSP, &fl);  // F_ALLOCSP is mapped to
                                           // F_ALLOCSP64 in 64-bit mode
    if (-1 == rc) {
        errorDescription << "Failed to fallocate file fd [" << fd
                         << "], rc: " << rc << ", errno: " << errno << " ["
                         << bsl::strerror(errno) << "]";
        return rc_SYSCALL_FAILURE;  // RETURN
    }

    return rc_SUCCESS;

#elif defined(BSLS_PLATFORM_OS_DARWIN)
    fstore_t fst;
    fst.fst_flags   = F_ALLOCATECONTIG;  // contiguous block
    fst.fst_posmode = F_PEOFPOSMODE;     // allocate from EOF
    fst.fst_offset  = offset;
    fst.fst_length  = length;
    int rc          = ::fcntl(fd, F_PREALLOCATE, &fst);
    // *NOTE*: Upon success, rc of fcntl() may be non-zero.
    if (-1 == rc) {
        // File system might be too fragmented to allocate a contiguous block.
        // Try one more time with non-contiguous block.
        fst.fst_flags = F_ALLOCATEALL;
        rc            = ::fcntl(fd, F_PREALLOCATE, &fst);
        if (-1 == rc) {
            // Failure
            errorDescription << "Failed to fallocate file fd [" << fd
                             << "], rc: " << rc << ", errno: " << errno << " ["
                             << bsl::strerror(errno) << "]";
            return rc_SYSCALL_FAILURE;  // RETURN
        }
    }

    // Need ftruncate after fcntl() success on darwin
    if (0 != (rc = ::ftruncate(fd, length))) {
        errorDescription << "Failed to ftruncate file fd [" << fd
                         << "], rc: " << rc << ", errno: " << errno << " ["
                         << bsl::strerror(errno) << "]";
        return rc_MISC_FAILURE;  // RETURN
    }

    return rc_SUCCESS;

#else
    // As of this writing, AIX has no support for fallocate-equivalent
    // functionality.
    return rc_UNSUPPORTED;

#endif
}

void FileSystemUtil::madvise(void*               mapping,
                             bsls::Types::Uint64 size,
                             int                 advice)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(mapping);

    ::madvise(static_cast<char*>(mapping), size, advice);
}

int FileSystemUtil::flush(void*               mapping,
                          bsls::Types::Uint64 size,
                          bsl::ostream&       errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(mapping);

    enum { rc_SUCCESS = 0, rc_MSYNC_FAILURE = -1 };

    int rc = ::msync(mapping, size, MS_SYNC);
    if (0 != rc) {
        errorDescription << "Failed to msync memory segment [" << mapping
                         << "] of size [" << size << "] bytes, rc: " << rc
                         << ", errno: " << errno << " ["
                         << bsl::strerror(errno) << "]";
        return rc_MSYNC_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

void FileSystemUtil::disableDump(void* mapping, bsls::Types::Uint64 size)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(mapping);

    // Indicate the OS not to dump these mappings in the core file.  Of all the
    // unix flavor that we care about, this flag is only available in linux 3.4
    // and above.  We expect all of linux boxes to have a system-level config
    // to exclude mapped pages from the core, but if support is available, we
    // still explicitly notify the OS to be on the safer side.

#if defined(BSLS_PLATFORM_OS_LINUX) && defined(MADV_DONTDUMP)
    madvise(mapping, size, MADV_DONTDUMP);
#endif

    (void)mapping;
    (void)size;  // Compiler happiness
}

}  // close package namespace
}  // close enterprise namespace
