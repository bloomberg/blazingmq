// Copyright 2022-2023 Bloomberg Finance L.P.
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

// bmqu_blob.cpp                                                      -*-C++-*-
#include <bmqu_blob.h>

#include <bmqscm_version.h>
// BDE
#include <bdlbb_blobutil.h>
#include <bsl_algorithm.h>
#include <bsl_cstring.h>
#include <bsl_memory.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace bmqu {

// ------------------
// class BlobPosition
// ------------------

bsl::ostream&
BlobPosition::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("buffer", d_buffer);
    printer.printAttribute("byte", d_byte);
    printer.end();

    return stream;
}

// -----------------
// class BlobSection
// -----------------

bsl::ostream&
BlobSection::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("start", d_start);
    printer.printAttribute("end", d_end);
    printer.end();
    return stream;
}

// --------------
// class BlobUtil
// --------------

bool BlobUtil::isValidPos(const bdlbb::Blob& blob, const BlobPosition& pos)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(pos.buffer() >
                                              blob.numDataBuffers())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(pos.buffer() ==
                                              blob.numDataBuffers())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Buffer points to one past the last buffer, valid only if pointing at
        // the first byte.
        return pos.byte() == 0;  // RETURN
    }

    return pos.byte() >= 0 && pos.byte() < bufferSize(blob, pos.buffer());
}

int BlobUtil::findOffset(BlobPosition*       pos,
                         const bdlbb::Blob&  blob,
                         const BlobPosition& start,
                         int                 offset)
{
    if (offset == 0) {
        *pos = start;
        return 0;  // RETURN
    }

    // Modify offset so we can look starting from the beginning of start
    // buffer
    offset += start.byte();

    const int numBuffers = blob.numDataBuffers();
    for (int bufferIdx = start.buffer(); bufferIdx < numBuffers; ++bufferIdx) {
        const int bufSize = bufferSize(blob, bufferIdx);
        if (offset < bufSize) {
            pos->setBuffer(bufferIdx);
            pos->setByte(offset);
            return 0;  // RETURN
        }
        else if (offset == bufSize) {
            // Make 'pos' point to the start of the next buffer.  This is
            // necessary so this loop works with the last data buffer
            pos->setBuffer(bufferIdx + 1);
            pos->setByte(0);
            return 0;  // RETURN
        }
        else {
            offset -= bufSize;
        }
    }

    // Looking past the end of the blob (return -3 since the 'safe' version
    // already uses -1 for invalid 'start' and -2 for invalid 'offset').
    return -3;
}

bool BlobUtil::isValidSection(const bdlbb::Blob& blob,
                              const BlobSection& section)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(section.end() <
                                              section.start())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !isValidPos(blob, section.start()))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !isValidPos(blob, section.end()))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }

    return true;
}

int BlobUtil::sectionSize(int*               size,
                          const bdlbb::Blob& blob,
                          const BlobSection& section)
{
    BSLS_ASSERT_SAFE(size);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !isValidSection(blob, section))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -1;  // RETURN
    }

    BlobPosition pos(section.start());

    *size = 0;

    while (pos.buffer() < section.end().buffer()) {
        *size += bufferSize(blob, pos.buffer()) - pos.byte();
        pos.setBuffer(pos.buffer() + 1);
        pos.setByte(0);
    }

    *size += section.end().byte() - pos.byte();

    return 0;
}

int BlobUtil::positionToOffset(int*                offset,
                               const bdlbb::Blob&  blob,
                               const BlobPosition& position)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(offset);

    *offset = 0;
    for (int bufferIdx = 0; bufferIdx < position.buffer(); ++bufferIdx) {
        *offset += bufferSize(blob, bufferIdx);
    }

    *offset += position.byte();

    return 0;
}

void BlobUtil::reserve(BlobPosition* pos, bdlbb::Blob* blob, int length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(length > 0);

    const int numBuffers           = blob->numDataBuffers();
    const int lastDataBufferLength = blob->lastDataBufferLength();

    if (numBuffers == 0 ||
        blob->buffer(numBuffers - 1).size() == lastDataBufferLength) {
        pos->setBuffer(numBuffers);
        pos->setByte(0);
    }
    else {
        pos->setBuffer(numBuffers - 1);
        pos->setByte(lastDataBufferLength);
    }

    blob->setLength(blob->length() + length);
}

int BlobUtil::writeBytes(const bdlbb::Blob*  blob,
                         const BlobPosition& pos,
                         const char*         buf,
                         int                 length)
{
    // PRECONDITIONS
    // Ensure that the position + length byte fits within the blob.
    BlobPosition end;
    const int    ret = findOffsetSafe(&end, *blob, pos, length);
    if (ret) {
        return (ret * 10) - 1;  // RETURN
    }

    BlobPosition curPos(pos);
    while (length) {
        const int toWrite = bsl::min(length,
                                     bufferSize(*blob, curPos.buffer()) -
                                         curPos.byte());
        bsl::memcpy(blob->buffer(curPos.buffer()).data() + curPos.byte(),
                    buf,
                    toWrite);
        curPos.setBuffer(curPos.buffer() + 1);
        curPos.setByte(0);
        buf += toWrite;
        length -= toWrite;
    }

    return 0;
}

void BlobUtil::appendBlobFromIndex(bdlbb::Blob*       destination,
                                   const bdlbb::Blob& source,
                                   int                startBufferIndex,
                                   int                offsetInBuffer,
                                   int                length)
{
    int                      offsetInBufferInt = offsetInBuffer;
    int                      dataToCopy        = length;
    const bdlbb::BlobBuffer* buffer = &source.buffer(startBufferIndex);
    while (dataToCopy) {
        const int bufferSize = buffer->size();
        if (0 < bufferSize) {
            int copyLength = bsl::min(bufferSize - offsetInBufferInt,
                                      dataToCopy);
            bdlbb::BlobUtil::append(destination,
                                    buffer->data() + offsetInBufferInt,
                                    copyLength);
            dataToCopy -= copyLength;
            offsetInBufferInt = 0;
        }
        ++buffer;
    }
}

int BlobUtil::appendToBlob(bdlbb::Blob*       dest,
                           const bdlbb::Blob& src,
                           const BlobSection& section)
{
    dest->trimLastDataBuffer();

    BlobSection localSection(section);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValidSection(src, section))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -1;  // RETURN
    }

    // Extend the last buffer of 'dest', if possible.
    if ((localSection.start() != localSection.end()) &&
        dest->numDataBuffers() > 0) {
        const BlobPosition&      start   = localSection.start();
        const bdlbb::BlobBuffer& destBuf = dest->buffer(
            dest->numDataBuffers() - 1);
        const bdlbb::BlobBuffer& srcBuf = src.buffer(start.buffer());

        const char* destEnd  = destBuf.data() + destBuf.size();
        const char* srcStart = srcBuf.data() + start.byte();

        // Check if src directly follows dest, and they both have the same
        // shared ptr rep.  If they don't have the same rep we can't just
        // extend dest because then we might end up using data that might be
        // unallocated later, and we'd have one hell of a bug to track down.
        if ((destEnd == srcStart) &&
            (srcBuf.buffer().rep() == destBuf.buffer().rep())) {
            // We can just extend dest's last data buffer, and then append the
            // rest
            const int srcBufLen = bufferSize(src, start.buffer()) -
                                  start.byte();

            int       secSize = 0;
            const int ret     = sectionSize(&secSize, src, localSection);
            if (ret != 0) {
                return (ret * 10) - 2;  // RETURN
            }

            const int extendLen = bsl::min(srcBufLen, secSize);

            bdlbb::BlobBuffer newBuf(destBuf);
            newBuf.setSize(newBuf.size() + extendLen);
            dest->removeBuffer(dest->numDataBuffers() - 1);
            dest->appendDataBuffer(newBuf);

            const int rc = findOffset(&localSection.start(),
                                      src,
                                      localSection.start(),
                                      extendLen);
            if (rc != 0) {
                return -2;  // RETURN
            }
        }
    }

    // Now we just have to append new buffers to 'dest'
    while (localSection.start().buffer() < localSection.end().buffer()) {
        const BlobPosition&         pos    = localSection.start();
        const bdlbb::BlobBuffer&    srcBuf = src.buffer(pos.buffer());
        const bsl::shared_ptr<char> buf(srcBuf.buffer(),
                                        srcBuf.data() + pos.byte());
        const bdlbb::BlobBuffer     newBuf(buf,
                                       bufferSize(src, pos.buffer()) -
                                           pos.byte());
        dest->appendDataBuffer(newBuf);

        localSection.start().setBuffer(localSection.start().buffer() + 1);
        localSection.start().setByte(0);
    }

    // Append the last, possibly partial buffer
    if (localSection.start() < localSection.end()) {
        const bdlbb::BlobBuffer& buf = src.buffer(
            localSection.start().buffer());
        const int startPos = localSection.start().byte();
        const int endPos   = localSection.end().byte();
        const int size     = endPos - startPos;

        if (startPos == 0) {
            // Can use the same SharedPtr
            const bdlbb::BlobBuffer appendBuf(buf.buffer(), size);
            dest->appendDataBuffer(appendBuf);
        }
        else {
            // Have to chop off the beginning
            const bsl::shared_ptr<char> srcBuf(buf.buffer(),
                                               buf.data() + startPos);
            const bdlbb::BlobBuffer     appendBuf(srcBuf, size);
            dest->appendDataBuffer(appendBuf);
        }
    }

    return 0;
}

int BlobUtil::appendToBlob(bdlbb::Blob*        dest,
                           const bdlbb::Blob&  src,
                           const BlobPosition& start,
                           int                 length)
{
    BSLS_ASSERT_SAFE(dest);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValidPos(src, start))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -1;  // RETURN
    }

    BlobSection section;
    section.start() = start;
    int ret         = findOffset(&section.end(), src, start, length);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(ret != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return (ret * 10) - 2;  // RETURN
    }

    ret = appendToBlob(dest, src, section);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(ret != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return (ret * 10) - 3;  // RETURN
    }

    return 0;
}

int BlobUtil::appendToBlob(bdlbb::Blob*        dest,
                           const bdlbb::Blob&  src,
                           const BlobPosition& start)
{
    BSLS_ASSERT_SAFE(dest);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValidPos(src, start))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -1;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(src.numDataBuffers() <= 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 0;  // RETURN
    }

    BlobSection section;
    section.start() = start;
    section.end().setBuffer(src.numDataBuffers());
    section.end().setByte(0);

    const int ret = appendToBlob(dest, src, section);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(ret != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return (ret * 10) - 2;  // RETURN
    }

    return 0;
}

void BlobUtil::copyToRawBufferFromIndex(char*              destination,
                                        const bdlbb::Blob& source,
                                        int                startBufferIndex,
                                        int                offsetInBuffer,
                                        int                length)
{
    BlobPosition curPos(startBufferIndex, offsetInBuffer);

    while (length != 0) {
        const bdlbb::BlobBuffer& buffer = source.buffer(curPos.buffer());
        const int buffSize              = bufferSize(source, curPos.buffer());

        if (0 < buffSize) {
            const int toCopy = bsl::min(buffSize - curPos.byte(), length);
            bsl::memcpy(destination, buffer.data() + curPos.byte(), toCopy);
            destination += toCopy;
            length -= toCopy;
        }
        curPos.setBuffer(curPos.buffer() + 1);
        curPos.setByte(0);
    }
}

int BlobUtil::compareSection(int*                cmpResult,
                             const bdlbb::Blob&  blob,
                             const BlobPosition& pos,
                             const char*         data,
                             int                 length)
{
    BlobPosition p(pos);
    while (length > 0 && p.buffer() < blob.numDataBuffers()) {
        const int bufLen = bufferSize(blob, p.buffer()) - p.byte();
        const int cmpLen = bsl::min(bufLen, length);
        const int cmpRet = bsl::memcmp(data,
                                       blob.buffer(p.buffer()).data() +
                                           p.byte(),
                                       cmpLen);
        if (cmpRet != 0) {
            *cmpResult = cmpRet < 0 ? -1 : 1;
            return 0;  // RETURN
        }

        data += cmpLen;
        length -= cmpLen;
        p.setBuffer(p.buffer() + 1);
        p.setByte(0);
    }

    if (length > 0) {
        return -1;  // RETURN
    }

    *cmpResult = 0;
    return 0;
}

int BlobUtil::readUpToNBytes(char*               buf,
                             const bdlbb::Blob&  blob,
                             const BlobPosition& start,
                             int                 length)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValidPos(blob, start))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -1;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            start.buffer() == blob.numDataBuffers() && length > 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -2;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(length == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 0;  // RETURN
    }

    char*              outPosition = buf;
    const int          numBuffers  = blob.numDataBuffers();
    bmqu::BlobPosition cursor(start);

    while (length != 0 && (cursor.buffer() != numBuffers)) {
        const bdlbb::BlobBuffer& buffer   = blob.buffer(cursor.buffer());
        const int                buffSize = bufferSize(blob, cursor.buffer());

        const int toCopy = bsl::min(buffSize - cursor.byte(), length);
        bsl::memcpy(outPosition, buffer.data() + cursor.byte(), toCopy);

        outPosition += toCopy;
        length -= toCopy;

        cursor.setBuffer(cursor.buffer() + 1);
        cursor.setByte(0);
    }

    return static_cast<int>(outPosition - buf);
}

int BlobUtil::readNBytes(char*               buf,
                         const bdlbb::Blob&  blob,
                         const BlobPosition& start,
                         int                 length)
{
    const int ret = readUpToNBytes(buf, blob, start, length);

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(ret == length)) {
        return 0;  // RETURN
    }
    else if (ret < 0) {
        return (ret * 10) - 1;  // RETURN
    }
    else {
        return -2;  // RETURN
    }
}

char* BlobUtil::getAlignedSectionSafe(char*               storage,
                                      const bdlbb::Blob&  blob,
                                      const BlobPosition& start,
                                      int                 length,
                                      int                 alignment,
                                      bool                copyFromBlob)
{
    BlobPosition end;
    int          ret = findOffsetSafe(&end, blob, start, length);
    if (ret) {
        return 0;  // RETURN
    }

    char* startPos = blob.buffer(start.buffer()).data() + start.byte();

    if (((start.buffer() == end.buffer()) ||
         (start.buffer() + 1 == end.buffer() && end.byte() == 0)) &&
        (bsls::AlignmentUtil::calculateAlignmentOffset(startPos, alignment) ==
         0)) {
        // Section is good
        return startPos;
    }

    // Have to copy
    if (copyFromBlob) {
        ret = readNBytes(storage, blob, start, length);
        if (ret) {
            return 0;  // RETURN
        }
    }

    return storage;
}

char* BlobUtil::getAlignedSection(char*               storage,
                                  const bdlbb::Blob&  blob,
                                  const BlobPosition& start,
                                  int                 length,
                                  int                 alignment,
                                  bool                copyFromBlob)
{
    BlobPosition end;
    int          ret = findOffset(&end, blob, start, length);
    if (ret) {
        return 0;  // RETURN
    }

    char* startPos = blob.buffer(start.buffer()).data() + start.byte();

    if (((start.buffer() == end.buffer()) ||
         (start.buffer() + 1 == end.buffer() && end.byte() == 0)) &&
        (bsls::AlignmentUtil::calculateAlignmentOffset(startPos, alignment) ==
         0)) {
        // Section is good
        return startPos;
    }

    // Have to copy
    if (copyFromBlob) {
        ret = readNBytes(storage, blob, start, length);
        if (ret) {
            return 0;  // RETURN
        }
    }

    return storage;
}

// ------------------------
// class BlobStartHexDumper
// -----------------------

// CREATORS
BlobStartHexDumper::BlobStartHexDumper(const bdlbb::Blob* blob)
: d_blob_p(blob)
, d_length(1024)
{
}

BlobStartHexDumper::BlobStartHexDumper(const bdlbb::Blob* blob, int length)
: d_blob_p(blob)
, d_length(length)
{
}

}  // close package namespace

// FREE OPERATORS
bsl::ostream& bmqu::operator<<(bsl::ostream&                   stream,
                               const bmqu::BlobStartHexDumper& src)
{
    if (src.d_blob_p->totalSize() == 0) {
        // Workaround for {internal-ticket D165124836} BDE assertion in
        // 'bdlbb::BlobUtilHexDumper' when dumping a Blob containing a single
        // zero-size BlobBuffer.  Just print nothing (which is what would
        // happen anyway) and return.
        return stream;  // RETURN
    }

    if (src.d_blob_p->length() <= src.d_length) {
        // Dump the whole thing
        return stream << bdlbb::BlobUtilHexDumper(src.d_blob_p);  // RETURN
    }
    else {
        // Dump only the first 'd_length' bytes
        stream << bdlbb::BlobUtilHexDumper(src.d_blob_p, 0, src.d_length);
        int remaining = src.d_blob_p->length() - src.d_length;
        return stream << "\t+ " << remaining << " more bytes. ("
                      << src.d_blob_p->length() << " total)";
    }
}

}  // close enterprise namespace
