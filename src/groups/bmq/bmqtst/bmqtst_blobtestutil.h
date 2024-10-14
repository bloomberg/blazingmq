// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqtst_blobtestutil.h                                              -*-C++-*-
#ifndef INCLUDED_BMQTST_BLOBTESTUTIL
#define INCLUDED_BMQTST_BLOBTESTUTIL

//@PURPOSE: Provide blob utilities for use in test drivers.
//
//@CLASSES:
//  bmqtst::BlobTestUtil: namespace for a set of blob utilities
//
//@DESCRIPTION: 'bmqtst::BlobTestUtil' provides a set of blob utilities to
// assist in writing test drivers.
//
/// Usage Example
///-------------
// This section illustrates intended use of this component.
//
/// Example 1: Blob from string
///- - - - - - - - - - - - - -
// The following code illustrates how to construct a blob from a string while
// specifying the internal breakdown of the blob buffers in the string.
//
//..
//  bdlbb::Blob blob(s_allocator_p);
//  bmqtst::BlobTestUtil::fromString(&blob, "a|b", s_allocator_p);
//
//  ASSERT_EQ(blob.length(),         2);
//  ASSERT_EQ(blob.numDataBuffers(), 2);
//
//  // First buffer
//  bsl::string buf1(blob.buffer(0).data(), 1U);
//  ASSERT_EQ(blob.buffer(0).size(), 1);
//  ASSERT_EQ(buf1,                  "a");
//
//  // Second buffer
//  bsl::string buf2(blob.buffer(1).data(), 1U);
//  ASSERT_EQ(blob.buffer(1).size(), 1);
//  ASSERT_EQ(buf2,                  "b");
//..
//
/// Example 2: Blob to string
///- - - - - - - - - - - - -
// The following code illustrates how to convert a blob to a string.
//
// First, let's put the string 'abcdefg' into the blob.
//..
//  bdlbb::Blob blob(s_allocator_p);
//  bmqtst::BlobTestUtil::fromString(&blob, "abcdefg", s_allocator_p);
//  BSLS_ASSERT_OPT(blob.length() == 7);
//  BSLS_ASSERT_OPT(blob.numDataBuffers() == 1);
//..
// Finally, we can convert the blob to a string using the 'toString' method.
//..
//  bsl::string str(s_allocator_p);
//  ASSERT_EQ("abcdefg", bmqtst::BlobTestUtil::toString(&str, blob));
//..

// BDE
#include <bdlbb_blob.h>
#include <bsl_string.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace bmqtst {

// ===================
// struct BlobTestUtil
// ===================

/// Struct containing various blob functions useful in test drivers.
struct BlobTestUtil {
    // CLASS METHODS

    /// Populate and return the specified `blob` according to the specified
    /// `format` using the optionally specified `allocator` to supply
    /// memory.  The bytes of the blob will contain the characters of the
    /// string, with the following characters having the following meaning:
    /// - `|`: Indicates a blob buffer boundary.  For example, "a|b" will
    ///        create a blob with two blob buffers, with 1 character each,
    ///        while "ab" will create a blob with one blob buffer, with the
    ///        two characters `a` and `b`.
    /// - `X`: Indicates how many bytes at the end of the blob should be
    ///        part of the last blob buffer's size, but not included in the
    ///        length of the blob.  For example, "aXX" will create a blob
    ///        with one blob buffer, which has a size of 3, but only the
    ///        first byte is part of the blob's length (so the blob's length
    ///        is `1`).
    ///
    /// Note that this method does not validate the input `format`.  Also
    /// note that "||" is not allowed.
    static bdlbb::Blob& fromString(bdlbb::Blob*             blob,
                                   const bslstl::StringRef& format,
                                   bslma::Allocator*        allocator = 0);

    /// Populate and return the specified `str` with the string
    /// representation of the data contained in the specified `blob`.  If
    /// the optionally specified `toFormat` is true, format the output
    /// string.  If so, the characters of the string will contain the bytes
    /// of the blob, with the following characters having the following
    /// meaning:
    /// - `|`: Indicates a blob buffer boundary.  For example, "a|b" will
    ///        correspond to a blob with two blob buffers, with 1 character
    ///        each, while "ab" will correspond to a blob with one blob
    ///        buffer, with the two characters `a` and `b`.
    /// - `X`: Indicates how many bytes at the end of the blob are part of
    ///        the last blob buffer's size, but not included in the length
    ///        of the blob.  For example, "aXX" will correspond to a blob
    ///        with one blob buffer, which has a size of 3, but only the
    ///        first byte is part of the blob's length (so the blob's length
    ///        is `1`).
    static bsl::string&
    toString(bsl::string* str, const bdlbb::Blob& blob, bool toFormat = false);
};

}  // close package namespace
}  // close enterprise namespace

#endif
