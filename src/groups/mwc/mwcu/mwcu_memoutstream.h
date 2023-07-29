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

// mwcu_memoutstream.h                                                -*-C++-*-
#ifndef INCLUDED_MWCU_MEMOUTSTREAM
#define INCLUDED_MWCU_MEMOUTSTREAM

//@PURPOSE: provides an 'ostream' exposing a StringRef to its internal buffer.
//
//@CLASSES:
//   MemOutStream: an 'ostream' exposing a StringRef to its internal buffer
//
//@SEE ALSO: bsl::ostringstream
//
//@DESCRIPTION: 'MemOutStream' provides the same service as
// 'bsl::ostringstream' but its 'str' method returns 'bslstl::StringRef'
// instead of 'bsl::string' (by value) so an unnecessary copy isn't made.  Also
// allows user to reserve buffer space, and easily clear the buffer to reuse
// for new output.
//
/// Usage
///-----
// This section illustrates intended usage of this component.
//
/// Example 1: Pipe a string into the buffer
///  - - - - - - - - - - - - - - - - - - - -
//
//..
//  MemOutStream oms(30);
//  oms << "hello world";
//  assert(os.str() == "hello world");
//..

#include <bdlsb_memoutstreambuf.h>
#include <bsl_cstddef.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace mwcu {

// ===============================
// class MemOutStream_BufferHolder
// ===============================

/// Private type used to construct the buffer before calling the constructor
/// of `bsl::ostream`, to which the buffer must be passed.
class MemOutStream_BufferHolder {
  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    MemOutStream_BufferHolder(const MemOutStream_BufferHolder&);  // = delete
    MemOutStream_BufferHolder&
    operator=(const MemOutStream_BufferHolder&);  // = delete

  protected:
    // PROTECTED DATA
    bdlsb::MemOutStreamBuf d_buffer;

    // PROTECTED CREATORS

    /// Constructs the internal buffer.  Uses the specified `allocator` to
    /// allocate memory.
    explicit MemOutStream_BufferHolder(bslma::Allocator* allocator);

    /// Constructs the internal buffer reserving the specified `initialSize`
    /// bytes.  Uses the specified `allocator` to allocate memory.
    MemOutStream_BufferHolder(bsl::size_t       initialSize,
                              bslma::Allocator* allocator);
};

// ==================
// class MemOutStream
// ==================

/// This class is used just like `bsl::ostringstream` except that the `str`
/// method returns a `bslstl::StringRef` instead of `bsl::string`.  It is
/// also possible to reserve memory in the underlying buffer.
class MemOutStream : private virtual MemOutStream_BufferHolder,
                     public bsl::ostream {
  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    MemOutStream(const MemOutStream&);             // = delete
    MemOutStream& operator=(const MemOutStream&);  // = delete

  public:
    // CREATORS

    /// Create a `MemOutStream` object.  Use the optionally specified
    /// `allocator` to allocate memory.  If the optionally specified
    /// `initialSize` is provided, reserve this many bytes internally for
    /// the stream output.
    explicit MemOutStream(bslma::Allocator* allocator = 0);
    explicit MemOutStream(bsl::size_t       initialSize,
                          bslma::Allocator* allocator = 0);

    /// Destroy this object.  Note that this member is defined (even though
    /// it is empty) and defined out of line for several good reasons.
    ~MemOutStream() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Reserves at least the specified `bytes` in the internal buffer.
    void reserveCapacity(bsl::size_t bytes);

    /// Resets the internal buffer to be empty so that the `str` method will
    /// return an empty string reference.  Note that this function does not
    /// clear the flags of the stream nor its "sticky" formatting.  If you
    /// need to clear those, you should utilize `OutStreamFormatSaver` or
    /// use the `bsl::ostream` interface of the object (as usual for any
    /// stream).
    void reset();

    // ACCESSORS

    /// Return a string reference representing the content of this stream's
    /// buffer, in other words, the output of this stream.
    bslstl::StringRef str() const;

    /// Return the number of valid characters in the buffer of this stream
    /// object.  Note that `obj.length() == obj.str.length()`.
    bsl::size_t length() const;

    /// Return `true` if the buffer of this stream object has no characters
    /// in it, and `false` otherwise.  Note that 'obj.isEmpty() ==
    /// obj.str.isEmpty() == (obj.length() == 0)'.
    bool isEmpty() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------------
// class MemOutStream_BufferHolder
// -------------------------------
inline MemOutStream_BufferHolder::MemOutStream_BufferHolder(
    bslma::Allocator* allocator)
: d_buffer(allocator)
{
    // NOTHING
}

inline MemOutStream_BufferHolder::MemOutStream_BufferHolder(
    bsl::size_t       initialSize,
    bslma::Allocator* allocator)
: d_buffer(initialSize, allocator)
{
    // NOTHING
}

// ------------------
// class MemOutStream
// ------------------

inline MemOutStream::MemOutStream(bslma::Allocator* allocator)
: MemOutStream_BufferHolder(allocator)
, bsl::ostream(&d_buffer)
{
    // NOTHING
}

inline MemOutStream::MemOutStream(bsl::size_t       initialSize,
                                  bslma::Allocator* allocator)
: MemOutStream_BufferHolder(initialSize, allocator)
, bsl::ostream(&d_buffer)
{
    // NOTHING
}

inline void MemOutStream::reserveCapacity(bsl::size_t bytes)
{
    d_buffer.reserveCapacity(bytes);
}

inline void MemOutStream::reset()
{
    d_buffer.reset();
}

inline bslstl::StringRef MemOutStream::str() const
{
    return bslstl::StringRef(d_buffer.data(), d_buffer.length());
}

inline bsl::size_t MemOutStream::length() const
{
    return d_buffer.length();
}

inline bool MemOutStream::isEmpty() const
{
    return d_buffer.length() == 0;
}

}  // close package namespace
}  // close enterprise namespace

#endif
