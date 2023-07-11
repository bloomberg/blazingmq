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

// mwcu_blobobjectproxy.h                                             -*-C++-*-
#ifndef INCLUDED_MWCU_BLOBOBJECTPROXY
#define INCLUDED_MWCU_BLOBOBJECTPROXY

//@PURPOSE: Provide a utility for reading/writing structures in 'bdlbb::Blob's
//
//@CLASSES:
// mwcu::BlobObjectProxy
//
//@SEE_ALSO:
// mwcu_blob
//
//@DESCRIPTION: This component defines a mechanism, 'mwcu::BlobObjectProxy',
// which allows the user to efficiently read and modify sections of a blob by
// treating them as a POD struct.  This is useful when processing binary
// messages which are often created by apppending together various headers
// defined by POD structs.
//
// A 'BlobObjectProxy' makes these operations as simple and efficient as
// possible by:
//: o casting directly into the blob if the entire requested object is
//:   contained in a single blob buffer and if it is aligned properly
//: o implementing 'operator->', allowing the user to treat the
//:   'BlobObjectProxy' as a pointer to the object in the blob
//
/// Thread Safety
///-------------
// Instances of 'BlobObjectProxy are *const* thread-safe.
//
/// Protocol evolution
///------------------
// Binary protocols typically evolve by growing the sizes of their various
// header structures.  The way this is done is by including in a higher-level
// header the size of another header or message section in the message.  Then
// a receiver of the message that doesn't know about the header extension can
// simply ignore the extra bytes and simply skip over them.
//
// Similarly, if a reader receives a message (from an older client, lets say)
// that omitted some of the fields it knows about, the reader should be able
// to process the message assuming those fields have some default values or
// possibly explicitly checking if the field was present in the message.
//
// The 'BlobObjectProxy' makes this kind of protocol evolution possible by
// optionally taking an object size at construction or in 'reset'.  If the
// size of the parameterized 'TYPE' is larger than the object in the blob,
// the 'BlobObjectProxy' will copy out the relevant data from the blob, zero
// out the excess in its buffer, and provide accessors allowing the user to
// find out if a given field was present in the message at all.
//
/// Determining Length
///------------------
// Often in binary protocols, the length of a structure is a member of the
// structure itself.  For example
//..
//  struct MessageHeder {
//      char                 d_headerLength;
//      char                 d_reserved[3];
//      bdlb::BigEndianInt32 d_someField;
//  };
//..
// When reading a blob containing a structure like this, what we'd like is an
// easy and efficient way of reading only the 'd_headerLength' field of the
// structure, and then using that to determine how much of the remainder of
// the structure we have.  To facilitate this, a 'BlobObjectProxy' can be
// created (or 'reset') with a negative length.  This indicates that it should
// read as much of its parameterized 'TYPE' as possible, ensuring that at
// least '-length' byes can actually be read, even if the object
// would extend past the end of the blob.  Once configured this way, the user
// can read the 'd_headerLength' field and 'resize' the 'BlobObjectProxy'
// appropriately.  It is imperative that when the 'BlobObjectProxy' is created
// in this way, that the user take care to only access fields that are
// guaranteed to exist in the structure in all versions of the protocol.
// Refer to the usage examples for an example of how this can be done.
//
/// Read/Write Access
///-----------------
// A 'BlobObjectProxy' can operate in read, write, or read/write mode, which
// affects how it handles the object when it has to be copied out of the blob;
// it has no effect if the object can be read/modified in the blob itself.
//
// If the 'BlobObjectProxy' is configured for reading, and the object can't
// simply be cast into the blob, the current contents of the object in the
// blob will be copied into the 'BlobObjectProxy's buffer and exposed to the
// application.  If the 'BlobObjectProxy' isn't created for reading, the
// contents of the 'BlobObjectProxy's buffer will be uninitialized.
//
// If a 'BlobObjectProxy' is configured for writing, the 'BlobObjectProxy's
// buffer will be written to the blob upon destruction or 'reset'.
//
/// Usage
///-----
// In this section we show the intended usage of this component.
//
// In the following examples, we will make use of this common POD type for our
// proxied blob object:
//..
//  struct TestHeader {
//      unsigned char        d_length;
//      unsigned char        d_reserved[3];
//      bdlb::BigEndianInt32 d_member1;
//      bdlb::BigEndianInt32 d_member2;
//  };
//..
//
/// Example 1: Reading objects from blobs
///- - - - - - - - - - - - - - - - - - -
// We demonstrate using a 'BlobObjectProxy' to read the fields of a proxied
// object in a blob.
//
// First, we initialize our blob by writing a single 'TestHeader' object to it
// with some test values.
//..
//  bdlbb::PooledBlobBufferFactory factory(0xFF);
//  bdlbb::Blob blob(&factory);
//
//  TestHeader hdr;
//  bsl::memset(&hdr, 0, sizeof(hdr));
//  hdr.d_length = sizeof(hdr);
//  hdr.d_member1 = 1;
//  hdr.d_member2 = 2;
//
//  bdlbb::BlobUtil::append(&blob, (char *)&hdr, sizeof(hdr));
//..
// Next, we can construct a 'BlobObjectProxy' for a 'TestHeader' object
// contained at the beginning of 'blob' and access its fields.
//..
//  BlobObjectProxy<TestHeader> pxy(&blob);
//  assert(pxy.isSet());
//  assert(pxy->d_length == sizeof(hdr));
//  assert(pxy->d_member1 == 1);
//  assert(pxy->d_member2 == 2);
//..
// Now, suppose we know that our blob only contains up to 'd_member1' of our
// 'TestHeader'.  We can reset (or construct) a 'BlobObjectProxy' with this
// limit, and it will correctly expose only the fields present in the blob.
//
// To do this, we can reset our proxy with a length of '8', which will allow
// us to read up to 'd_member1' of our structure.
//..
//  pxy.reset(&blob, 8);
//  assert(pxy->d_length == sizeof(hdr));
//  assert(pxy->d_member1 == 1);
//..
// When configured this way, the 'BlobObjectProxy' will zero out the remaining
// fields in its internal buffer (the 'blob' is not modified in any way).
//..
//  assert(pxy->d_member2 == 0);
//..
// In most cases, having the extraneous members return a value of '0' should
// be enough, however in some cases it might be desirable to explicitly check
// whether a field is actually present in the proxied object.  This can be
// accomplished using 'hasMember' as follows:
//..
//  assert(pxy.hasMember(&TestHeader::d_length));
//  assert(pxy.hasMember(&TestHeader::d_member1));
//  assert(!pxy.hasMember(&TestHeader::d_member2));
//..
// Finally, we will demonstrate the proper way of reading a blob object whose
// length is one of its members and can't be known until the object is already
// proxied.
//
// By configuring the 'BlobObjectProxy' with a length of '-1', we instruct it
// to read as much of our object from the blob as possible, without any
// knowledge (yet) of whether all the data is actually a part of our object.
//..
//  pxy.reset(&blob, -1);
//..
// At this point, 'pxy' is proxying as much of 'TestHeader' as possible, but
// we still need to ensure that 'd_length' is one of the proxied fields (for
// example, if the blob was only 2 bytes long).
//..
//  assert(pxy.hasMember(&TestHeader::d_length));
//..
// Now we can safely access 'd_length' in our object, and update the proxied
// object's size appropriately.
//..
//  pxy.resize(pxy->d_length);
//..
// Now, we can safely access any desired fields of our proxied object.  Note
// that accessing fields that were not actually present in the message using a
// 'BlobObjectProxy' before its length is set correctly will result in
// *undefined behavior*.
//
/// Example 2: Modify blob objects
///  - - - - - - - - - - - - - - -
// We demonstrate using a 'BlobObjectProxy' to modify a proxied blob object
// efficiently.  To do this, we need to create our blob with enough room to
// store an instance of our 'TestHeader'.
//..
//  bdlbb::PooledBlobBufferFactory factory(0xFF);
//  bdlbb::Blob blob(&factory);
//
//  TestHeader hdr;
//  bdlbb::BlobUtil::append(&blob, (char *)&hdr, sizeof(hdr));
//..
// With the blob in this uninitialized state, first, we must create a
// 'BlobObjectProxy' for writing.
//..
//  BlobObjectProxy<TestHeader> pxy(&blob,
//                                  false,  // don't read current contents
//                                  true);  // write out changes upon
//..
// Configured this way, the 'BlobObjectProxy' will write any changes made to
// the proxied object back into its blob if necessary when it is destroyed or
// reset.
//
// Now we can proceed to fill our object with values.
//..
//  bsl::memset(pxy.object(), 0, sizeof(TestHeader));
//  pxy->d_length = sizeof(TestHeader);
//  pxy->d_member1 = 17;
//  pxy->d_member2 = 32;
//  pxy.reset();
//..
// Finally, we verify that the blob contains our changes.
//..
//  bdlbb::Blob expectedBlob(&factory);
//
//  bsl::memset(&hdr, 0, sizeof(hdr));
//  hdr.d_length = sizeof(TestHeader);
//  hdr.d_member1 = 17;
//  hdr.d_member2 = 32;
//  bdlbb::BlobUtil::append(&expectedBlob, (char *)&hdr, sizeof(hdr));
//  assert(!bdlbb::BlobUtil::compare(blob, expectedBlob));
//..

// MWC

#include <mwcu_blob.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_cstring.h>
#include <bsls_objectbuffer.h>
#include <bsls_performancehint.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwcu {

// =====================
// class BlobObjectProxy
// =====================

/// Utility for reading/writing an object in a `bdlbb::Blob` easily, safely,
/// and efficiently.
template <class TYPE>
class BlobObjectProxy {
  private:
    // DATA
    bool d_readMode;  // are we configured for reading?

    bool d_writeMode;  // are we configured for writing?

    const bdlbb::Blob* d_blob_p;  // the blob our object resides in

    mwcu::BlobPosition d_position;  // the position of our object in
                                    // 'd_blob_p'

    int d_length;  // the length of the object in the
                   // blob

    bsls::ObjectBuffer<TYPE> d_buffer;  // buffer for our object if we
                                        // can't cast into the blob

    TYPE* d_object_p;  // pointer to our object, either
                       // in 'd_blob_p' or 'd_buffer'

    // PRIVATE MANIPULATORS

    /// Write out our object buffer to our blob if necessary.
    void writeOutIfNecessary();

    /// Reset this blob object.  Refer to the documentation of the
    /// constructor for a description of the arguments.
    bool resetImp(const bdlbb::Blob*        blob,
                  const mwcu::BlobPosition& position,
                  int                       length,
                  bool                      read,
                  bool                      write,
                  bool                      raw);

    // NOT IMPLEMENTED
    BlobObjectProxy(const BlobObjectProxy&);
    BlobObjectProxy& operator=(const BlobObjectProxy&);

  public:
    // CREATORS

    /// Create a `BlobObjectProxy` not referring to any blob object.
    BlobObjectProxy();

    /// Create a `BlobObjectProxy` referring to an instance of the
    /// parameterized `TYPE` at the optionally specified `position` in the
    /// specified `blob` using the optionally specified `length` to
    /// determine whether the whole object is present in the `blob`.  If
    /// `position` isn't provided, the start of `blob` is assumed.  If
    /// `length` isn't provided, `sizeof(TYPE)` is assumed.  If `length` < 0
    /// an attempt will be made to read a minimum of `-length` bytes from the
    /// blob and it will be the users responsibility to only access fields
    /// that are known to be valid; refer to the `Determining Length` section
    /// of the component-level documentation for more information.  The
    /// optionally specifed `read` and `write` determine whether the
    /// `BlobObjectProxy` is created in `read`, `write` or `read/write` mode.
    /// Refer to the component-level documentation for an explanation of the
    /// various modes.  If it is impossible to refer to the requested blob
    /// object (for example, if `position + length` extends past the end of
    /// the `blob), `isSet()` will return `false' when called on this object.
    BlobObjectProxy(const bdlbb::Blob* blob,
                    bool               read  = true,
                    bool               write = false);
    BlobObjectProxy(const bdlbb::Blob* blob,
                    int                length,
                    bool               read  = true,
                    bool               write = false);
    BlobObjectProxy(const bdlbb::Blob*        blob,
                    const mwcu::BlobPosition& position,
                    bool                      read  = true,
                    bool                      write = false);
    BlobObjectProxy(const bdlbb::Blob*        blob,
                    const mwcu::BlobPosition& position,
                    int                       length,
                    bool                      read  = true,
                    bool                      write = false);

    /// Destroy this object, writing out the internal object buffer to our
    /// blob if necessary.
    ~BlobObjectProxy();

    // MANIPULATORS

    /// Flush any buffered changed if necessary, and make this object not
    /// refer to any valid blob object.
    void reset();

    /// Reset this `BlobObjectProxy` with the specified `blob`, `position`,
    /// `length`, `read` and `write` which have the same meaning as the
    /// corresponding arguments to the constructor.  Flush the current
    /// changes if necessary.  Return `true` if the reset succeeded or
    /// `false` if it failed (for example, if `position + length` extends
    /// past the end of the `blob`).  If this returns `false`, `isSet()`
    /// will also return `false` until this object is reset again.
    bool reset(const bdlbb::Blob* blob, bool read = true, bool write = false);
    bool reset(const bdlbb::Blob* blob,
               int                length,
               bool               read  = true,
               bool               write = false);
    bool reset(const bdlbb::Blob*        blob,
               const mwcu::BlobPosition& position,
               bool                      read  = true,
               bool                      write = false);
    bool reset(const bdlbb::Blob*        blob,
               const mwcu::BlobPosition& position,
               int                       length,
               bool                      read  = true,
               bool                      write = false);

    /// Reset this `BlobObjectProxy` with the specified `blob`, `position`,
    /// `length`, `read` and `write` which have the same meaning as the
    /// corresponding arguments to the constructor.  Flush the current
    /// changes if necessary.  Return `true` if the reset succeeded or
    /// `false` if it failed.  If this returns `false`, `isSet()` will also
    /// return `false` until this object is reset again.  Note that this
    /// method is equivalent to `reset` except that it assumes that the
    /// parameters are correct vis-a-vis the blob.
    bool
    resetRaw(const bdlbb::Blob* blob, bool read = true, bool write = false);
    bool resetRaw(const bdlbb::Blob* blob,
                  int                length,
                  bool               read  = true,
                  bool               write = false);
    bool resetRaw(const bdlbb::Blob*        blob,
                  const mwcu::BlobPosition& position,
                  bool                      read  = true,
                  bool                      write = false);
    bool resetRaw(const bdlbb::Blob*        blob,
                  const mwcu::BlobPosition& position,
                  int                       length,
                  bool                      read  = true,
                  bool                      write = false);

    /// Resize this object to the specified `newLength`.  This is normally
    /// done with a `BlobObjectProxy` created or reset with a negative
    /// `length` once the true length of the object is determined.  Return
    /// `true` if the new length is valid, and `false` otherwise'.  If this
    /// returns `false`, `isSet` will also return `false` until this object
    /// is reset.
    bool resize(int newLength);

    /// Return a pointer to the object in our blob if possible.  Otherwise,
    /// return a pointer to an internal buffer.  The behavior is undefined
    /// unless `isSet()` returns `true`.  The behavior is also undefined if
    /// any fields of the returned object are modified and this
    /// `BlobObjectProxy` wasn't configured for writing.
    TYPE*       object();
    TYPE*       operator->();
    TYPE&       operator*();
    const TYPE* object() const;
    const TYPE* operator->() const;
    const TYPE& operator*() const;

    // ACCESSORS

    ///  Return the blob our object resides in.
    const bdlbb::Blob* blob() const;

    /// Return the position of our object in the blob.
    const mwcu::BlobPosition& position() const;

    /// Load into the specified `pos` the position in the `blob()` that is
    /// right after the end of the object being proxied.  This is
    /// especially useful when creating another `BlobObjectProxy` for an
    /// object immediately following the object being proxied by this
    /// `BlobObjectProxy`. The behavior is undefiend unless `isSet()`
    /// return `true`.
    void loadEndPosition(mwcu::BlobPosition* pos) const;

    /// Return the length of the object in the blob.
    int length() const;

    /// Return `true` if this object refers to a valid blob object and
    /// `false` otherwise.  The behavior of any function of this object
    /// aside from `reset` is undefined if this returns `false`.
    bool isSet() const;

    /// Return `true` if we were able to simply cast our object into our
    /// blob, and `false` if the proxied object is being accessed through
    /// this object's buffer.
    bool isInBlob() const;

    /// Return `true` if the specified `memberPtr` is a pointer-to-member
    /// of our parameterized `TYPE` that is included in the proxied portion
    /// of our object in the blob, and `false` otherwise'.  If we are
    /// proxying an object defined by the following struct:
    /// ```
    /// struct Header {
    ///     int d_length;
    ///     int d_member;
    /// };
    /// ```
    /// Then calling `hasMember(&Header::d_member)` on a
    /// `BlobObjectProxy<Header>` would return `true` if its current
    /// `length` was at least `2 * sizeof(int)` and `false` otherwise.
    template <class MEMBER_TYPE>
    bool hasMember(MEMBER_TYPE TYPE::*memberPtr) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class BlobObjectProxy
// ---------------------

// PRIVATE MANIPULATORS
template <class TYPE>
inline void BlobObjectProxy<TYPE>::writeOutIfNecessary()
{
    if (d_writeMode && (d_object_p == &d_buffer.object())) {
        mwcu::BlobUtil::writeBytes(d_blob_p,
                                   d_position,
                                   reinterpret_cast<char*>(&d_buffer.object()),
                                   d_length);
    }
}

template <class TYPE>
inline bool BlobObjectProxy<TYPE>::resetImp(const bdlbb::Blob*        blob,
                                            const mwcu::BlobPosition& position,
                                            int                       length,
                                            bool                      read,
                                            bool                      write,
                                            bool                      raw)
{
    d_readMode  = read;
    d_writeMode = write;
    d_blob_p    = blob;
    d_position  = position;
    d_object_p  = 0;

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(length >=
                                            static_cast<int>(sizeof(TYPE)))) {
        d_object_p = (raw)
                         ? mwcu::BlobUtil::getAlignedObject(&d_buffer.object(),
                                                            *blob,
                                                            position,
                                                            read)
                         : mwcu::BlobUtil::getAlignedObjectSafe(
                               &d_buffer.object(),
                               *blob,
                               position,
                               read);
        d_length   = sizeof(TYPE);
    }
    else if (length < 0) {
        d_object_p = (raw)
                         ? mwcu::BlobUtil::getAlignedObject(&d_buffer.object(),
                                                            *blob,
                                                            position,
                                                            read)
                         : mwcu::BlobUtil::getAlignedObjectSafe(
                               &d_buffer.object(),
                               *blob,
                               position,
                               read);
        if (d_object_p) {
            d_length = sizeof(TYPE);
        }
        else {
            d_length = mwcu::BlobUtil::readUpToNBytes(d_buffer.buffer(),
                                                      *blob,
                                                      position,
                                                      sizeof(TYPE));
            if (d_length >= -length) {
                d_object_p = &d_buffer.object();
            }
        }
    }
    else {
        const int ret = mwcu::BlobUtil::readNBytes(d_buffer.buffer(),
                                                   *blob,
                                                   position,
                                                   length);
        if (ret == 0) {
            bsl::memset(d_buffer.buffer() + length, 0, sizeof(TYPE) - length);
            d_object_p = &d_buffer.object();
            d_length   = length;
        }
    }

    return d_object_p;
}

// CREATORS
template <class TYPE>
inline BlobObjectProxy<TYPE>::BlobObjectProxy()
: d_readMode(false)
, d_writeMode(false)
, d_blob_p(0)
, d_position()
, d_length(0)
, d_buffer()
, d_object_p(0)
{
}

template <class TYPE>
inline BlobObjectProxy<TYPE>::BlobObjectProxy(const bdlbb::Blob* blob,
                                              bool               read,
                                              bool               write)
{
    resetImp(blob, mwcu::BlobPosition(), sizeof(TYPE), read, write, false);
}

template <class TYPE>
inline BlobObjectProxy<TYPE>::BlobObjectProxy(const bdlbb::Blob* blob,
                                              int                length,
                                              bool               read,
                                              bool               write)
{
    resetImp(blob, mwcu::BlobPosition(), length, read, write, false);
}

template <class TYPE>
inline BlobObjectProxy<TYPE>::BlobObjectProxy(
    const bdlbb::Blob*        blob,
    const mwcu::BlobPosition& position,
    bool                      read,
    bool                      write)
{
    resetImp(blob, position, sizeof(TYPE), read, write, false);
}

template <class TYPE>
inline BlobObjectProxy<TYPE>::BlobObjectProxy(
    const bdlbb::Blob*        blob,
    const mwcu::BlobPosition& position,
    int                       length,
    bool                      read,
    bool                      write)
{
    resetImp(blob, position, length, read, write, false);
}

template <class TYPE>
BlobObjectProxy<TYPE>::~BlobObjectProxy()
{
    writeOutIfNecessary();
}

// MANIPULATORS
template <class TYPE>
void BlobObjectProxy<TYPE>::reset()
{
    writeOutIfNecessary();

    d_readMode  = false;
    d_writeMode = false;
    d_blob_p    = 0;
    d_length    = 0;
    d_object_p  = 0;
}

template <class TYPE>
inline bool
BlobObjectProxy<TYPE>::reset(const bdlbb::Blob* blob, bool read, bool write)
{
    writeOutIfNecessary();
    return resetImp(blob,
                    mwcu::BlobPosition(),
                    sizeof(TYPE),
                    read,
                    write,
                    false);
}

template <class TYPE>
inline bool BlobObjectProxy<TYPE>::reset(const bdlbb::Blob* blob,
                                         int                length,
                                         bool               read,
                                         bool               write)
{
    writeOutIfNecessary();
    return resetImp(blob, mwcu::BlobPosition(), length, read, write, false);
}

template <class TYPE>
inline bool BlobObjectProxy<TYPE>::reset(const bdlbb::Blob*        blob,
                                         const mwcu::BlobPosition& position,
                                         bool                      read,
                                         bool                      write)
{
    writeOutIfNecessary();
    return resetImp(blob, position, sizeof(TYPE), read, write, false);
}

template <class TYPE>
inline bool BlobObjectProxy<TYPE>::reset(const bdlbb::Blob*        blob,
                                         const mwcu::BlobPosition& position,
                                         int                       length,
                                         bool                      read,
                                         bool                      write)
{
    writeOutIfNecessary();
    return resetImp(blob, position, length, read, write, false);
}

template <class TYPE>
inline bool
BlobObjectProxy<TYPE>::resetRaw(const bdlbb::Blob* blob, bool read, bool write)
{
    writeOutIfNecessary();
    return resetImp(blob,
                    mwcu::BlobPosition(),
                    sizeof(TYPE),
                    read,
                    write,
                    true);
}

template <class TYPE>
inline bool BlobObjectProxy<TYPE>::resetRaw(const bdlbb::Blob* blob,
                                            int                length,
                                            bool               read,
                                            bool               write)
{
    writeOutIfNecessary();
    return resetImp(blob, mwcu::BlobPosition(), length, read, write, true);
}

template <class TYPE>
inline bool BlobObjectProxy<TYPE>::resetRaw(const bdlbb::Blob*        blob,
                                            const mwcu::BlobPosition& position,
                                            bool                      read,
                                            bool                      write)
{
    writeOutIfNecessary();
    return resetImp(blob, position, sizeof(TYPE), read, write, true);
}

template <class TYPE>
inline bool BlobObjectProxy<TYPE>::resetRaw(const bdlbb::Blob*        blob,
                                            const mwcu::BlobPosition& position,
                                            int                       length,
                                            bool                      read,
                                            bool                      write)
{
    writeOutIfNecessary();
    return resetImp(blob, position, length, read, write, true);
}

template <class TYPE>
bool BlobObjectProxy<TYPE>::resize(int newLength)
{
    BSLS_ASSERT(d_object_p);

    if (d_object_p == &d_buffer.object()) {
        // We had to copy our object from the blob
        if (newLength < d_length) {
            // We need to zero out the remainder of the buffer
            bsl::memset(d_buffer.buffer() + newLength,
                        0,
                        sizeof(TYPE) - newLength);
            d_length = newLength;
        }
        else if (newLength > d_length) {
            return resetImp(d_blob_p,
                            d_position,
                            newLength,
                            d_readMode,
                            d_writeMode,
                            false);
        }

        return true;
    }
    else {
        // We're pointing directly into a buffer in the blob.
        // 'd_length == sizeof(TYPE)'.
        if (newLength > d_length) {
            // No-op since length can't be greater than sizeof(TYPE); and in
            // this case, 'd_length == sizeof(TYPE)' already.
            return true;
        }
        else if (newLength < d_length) {
            // We need to use our buffer since we'll need to zero out some
            // section of the object.
            return resetImp(d_blob_p,
                            d_position,
                            newLength,
                            d_readMode,
                            d_writeMode,
                            false);
        }

        return true;
    }
}

template <class TYPE>
TYPE* BlobObjectProxy<TYPE>::object()
{
    return d_object_p;
}

template <class TYPE>
inline TYPE* BlobObjectProxy<TYPE>::operator->()
{
    return d_object_p;
}

template <class TYPE>
inline TYPE& BlobObjectProxy<TYPE>::operator*()
{
    return *d_object_p;
}

template <class TYPE>
const TYPE* BlobObjectProxy<TYPE>::object() const
{
    return d_object_p;
}

template <class TYPE>
inline const TYPE* BlobObjectProxy<TYPE>::operator->() const
{
    return d_object_p;
}

template <class TYPE>
inline const TYPE& BlobObjectProxy<TYPE>::operator*() const
{
    return *d_object_p;
}

// ACCESSORS

template <class TYPE>
inline const bdlbb::Blob* BlobObjectProxy<TYPE>::blob() const
{
    return d_blob_p;
}

template <class TYPE>
inline const mwcu::BlobPosition& BlobObjectProxy<TYPE>::position() const
{
    return d_position;
}

template <class TYPE>
inline void
BlobObjectProxy<TYPE>::loadEndPosition(mwcu::BlobPosition* pos) const
{
    int ret = mwcu::BlobUtil::findOffset(pos, *d_blob_p, d_position, d_length);
    (void)ret;
    // This should never fail since 'isSet()' will only be true if there are
    // 'd_length' bytes in the blob.
    BSLS_ASSERT(ret == 0);
}

template <class TYPE>
inline int BlobObjectProxy<TYPE>::length() const
{
    return d_length;
}

template <class TYPE>
inline bool BlobObjectProxy<TYPE>::isSet() const
{
    return d_object_p;
}

template <class TYPE>
inline bool BlobObjectProxy<TYPE>::isInBlob() const
{
    return d_object_p != &d_buffer.object();
}

template <class TYPE>
template <class MEMBER_TYPE>
inline bool
BlobObjectProxy<TYPE>::hasMember(MEMBER_TYPE TYPE::*memberPtr) const
{
    BSLS_ASSERT(d_object_p);
    bsls::Types::IntPtr minNecessary =
        reinterpret_cast<bsls::Types::IntPtr>(&(d_object_p->*memberPtr)) -
        reinterpret_cast<bsls::Types::IntPtr>(d_object_p) +
        sizeof(MEMBER_TYPE);
    return d_length >= minNecessary;
}

}  // close package namespace
}  // close enterprise namespace

#endif
