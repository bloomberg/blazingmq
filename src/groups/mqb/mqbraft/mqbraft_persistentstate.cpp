// Copyright 2026 Bloomberg Finance L.P.
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

// mqbraft_persistentstate.cpp -*-C++-*-
#include <mqbraft_persistentstate.h>

// MQB
#include <mqbraft_raftnode.h>

// BMQ
#include <bmqp_crc32c.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdls_filesystemutil.h>
#include <bdls_pathutil.h>
#include <bsl_cstring.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>

// SYSTEM
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace BloombergLP {
namespace mqbraft {

namespace {

const unsigned int k_MAGIC   = 0x424D5152;  // "BMQR"
const unsigned int k_VERSION = 1;

/// The file contents, in network byte order.
struct FileImage {
    bdlb::BigEndianUint32 d_magic;
    bdlb::BigEndianUint32 d_version;
    bdlb::BigEndianUint64 d_currentTerm;
    bdlb::BigEndianUint64 d_voteTerm;
    bdlb::BigEndianInt32  d_votedFor;

    /// CRC32-C of every field above.
    bdlb::BigEndianUint32 d_crc32c;
};

const unsigned int k_IMAGE_SIZE   = 32;
const unsigned int k_CRC_COVERAGE = 28;  // every byte before 'd_crc32c'

BSLMF_ASSERT(sizeof(FileImage) == k_IMAGE_SIZE);

unsigned int imageCrc(const FileImage& image)
{
    return bmqp::Crc32c::calculate(&image, k_CRC_COVERAGE);
}

/// Write the specified `image` to the specified `fd`.  Return 0 on success,
/// and `errno` otherwise.
int writeImage(int fd, const FileImage& image)
{
    char buffer[k_IMAGE_SIZE];
    bsl::memcpy(buffer, &image, k_IMAGE_SIZE);

    unsigned int done = 0;
    while (done < k_IMAGE_SIZE) {
        const ssize_t n = ::write(fd, buffer + done, k_IMAGE_SIZE - done);
        if (n < 0) {
            if (errno == EINTR) {
                continue;  // CONTINUE
            }
            return errno;  // RETURN
        }
        done += n;
    }
    return 0;
}

/// Read up to `k_IMAGE_SIZE` bytes from the specified `fd` into the
/// specified `image`, and load into the specified `bytesRead` how many were
/// read.  Return 0 on success, and `errno` otherwise.
int readImage(FileImage* image, unsigned int* bytesRead, int fd)
{
    char buffer[k_IMAGE_SIZE];

    unsigned int done = 0;
    while (done < k_IMAGE_SIZE) {
        const ssize_t n = ::read(fd, buffer + done, k_IMAGE_SIZE - done);
        if (n < 0) {
            if (errno == EINTR) {
                continue;  // CONTINUE
            }
            return errno;  // RETURN
        }
        if (n == 0) {
            break;  // BREAK
        }
        done += n;
    }

    bsl::memcpy(image, buffer, done);
    *bytesRead = done;
    return 0;
}

}  // close unnamed namespace

// ----------------------
// struct PersistentState
// ----------------------

PersistentState::PersistentState()
: d_currentTerm(0)
, d_voteTerm(0)
, d_votedFor(RaftNode::k_INVALID_NODE_ID)
{
}

bsl::ostream& operator<<(bsl::ostream& stream, const PersistentState& value)
{
    return stream << "[currentTerm: " << value.d_currentTerm
                  << ", voteTerm: " << value.d_voteTerm
                  << ", votedFor: " << value.d_votedFor << "]";
}

// --------------------------
// struct PersistentStateUtil
// --------------------------

int PersistentStateUtil::load(PersistentState*   state,
                              bsl::ostream&      errorDescription,
                              const bsl::string& path)
{
    BSLS_ASSERT_SAFE(state);

    enum {
        rc_SUCCESS      = 0,
        rc_OPEN_FAILURE = -1,
        rc_READ_FAILURE = -2,
        rc_SHORT_FILE   = -3,
        rc_BAD_MAGIC    = -4,
        rc_BAD_VERSION  = -5,
        rc_BAD_CHECKSUM = -6
    };

    if (!bdls::FilesystemUtil::exists(path)) {
        *state = PersistentState();
        return rc_SUCCESS;  // RETURN
    }

    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        errorDescription << "Failed to open '" << path
                         << "', errno: " << errno;
        return rc_OPEN_FAILURE;  // RETURN
    }

    FileImage    image;
    unsigned int bytesRead = 0;
    const int    err       = readImage(&image, &bytesRead, fd);
    ::close(fd);

    if (err != 0) {
        errorDescription << "Failed to read '" << path << "', errno: " << err;
        return rc_READ_FAILURE;  // RETURN
    }
    if (bytesRead != k_IMAGE_SIZE) {
        errorDescription << "'" << path << "' holds " << bytesRead
                         << " bytes, expected " << k_IMAGE_SIZE;
        return rc_SHORT_FILE;  // RETURN
    }

    const unsigned int magic   = image.d_magic;
    const unsigned int version = image.d_version;
    const unsigned int crc     = image.d_crc32c;

    if (magic != k_MAGIC) {
        errorDescription << "'" << path << "' has a bad magic number";
        return rc_BAD_MAGIC;  // RETURN
    }
    if (version != k_VERSION) {
        errorDescription << "'" << path << "' has unsupported version "
                         << version;
        return rc_BAD_VERSION;  // RETURN
    }
    if (crc != imageCrc(image)) {
        errorDescription << "'" << path << "' fails its checksum";
        return rc_BAD_CHECKSUM;  // RETURN
    }

    state->d_currentTerm = image.d_currentTerm;
    state->d_voteTerm    = image.d_voteTerm;
    state->d_votedFor    = image.d_votedFor;

    return rc_SUCCESS;
}

int PersistentStateUtil::save(bsl::ostream&          errorDescription,
                              const bsl::string&     path,
                              const PersistentState& state)
{
    enum {
        rc_SUCCESS          = 0,
        rc_OPEN_FAILURE     = -1,
        rc_WRITE_FAILURE    = -2,
        rc_SYNC_FAILURE     = -3,
        rc_RENAME_FAILURE   = -4,
        rc_DIR_OPEN_FAILURE = -5,
        rc_DIR_SYNC_FAILURE = -6
    };

    FileImage image;
    bsl::memset(&image, 0, sizeof(image));
    image.d_magic       = bdlb::BigEndianUint32::make(k_MAGIC);
    image.d_version     = bdlb::BigEndianUint32::make(k_VERSION);
    image.d_currentTerm = bdlb::BigEndianUint64::make(state.d_currentTerm);
    image.d_voteTerm    = bdlb::BigEndianUint64::make(state.d_voteTerm);
    image.d_votedFor    = bdlb::BigEndianInt32::make(state.d_votedFor);
    image.d_crc32c      = bdlb::BigEndianUint32::make(imageCrc(image));

    bsl::string tmpPath(path);
    tmpPath.append(".tmp");

    const int fd = ::open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        errorDescription << "Failed to open '" << tmpPath
                         << "', errno: " << errno;
        return rc_OPEN_FAILURE;  // RETURN
    }

    int err = writeImage(fd, image);
    if (err != 0) {
        ::close(fd);
        errorDescription << "Failed to write '" << tmpPath
                         << "', errno: " << err;
        return rc_WRITE_FAILURE;  // RETURN
    }

    if (::fsync(fd) != 0) {
        err = errno;
        ::close(fd);
        errorDescription << "Failed to sync '" << tmpPath
                         << "', errno: " << err;
        return rc_SYNC_FAILURE;  // RETURN
    }
    ::close(fd);

    if (::rename(tmpPath.c_str(), path.c_str()) != 0) {
        errorDescription << "Failed to rename '" << tmpPath << "' to '" << path
                         << "', errno: " << errno;
        return rc_RENAME_FAILURE;  // RETURN
    }

    // The rename is durable only once the directory is synced.
    bsl::string dir;
    if (0 != bdls::PathUtil::getDirname(&dir, path) || dir.empty()) {
        dir = ".";
    }

    const int dirFd = ::open(dir.c_str(), O_RDONLY);
    if (dirFd < 0) {
        errorDescription << "Failed to open directory '" << dir
                         << "', errno: " << errno;
        return rc_DIR_OPEN_FAILURE;  // RETURN
    }
    if (::fsync(dirFd) != 0) {
        err = errno;
        ::close(dirFd);
        errorDescription << "Failed to sync directory '" << dir
                         << "', errno: " << err;
        return rc_DIR_SYNC_FAILURE;  // RETURN
    }
    ::close(dirFd);

    return rc_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
