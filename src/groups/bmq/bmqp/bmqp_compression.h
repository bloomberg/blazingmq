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

// bmqp_compression.h                                                 -*-C++-*-
#ifndef INCLUDED_BMQP_COMPRESSION
#define INCLUDED_BMQP_COMPRESSION

//@PURPOSE: Provide a utility for compression.
//
//@CLASSES:
//  bmqp::Compression     : contains top level functionality for compression.
//  bmqp::Compression_Impl: contains implementation of compression logic.
//
//@DESCRIPTION: This component defines a utility struct, 'bmqp::Compression',
// that provides functionality of 'compress', 'decompress' as per the specified
// 'algorithm'. It additionally defines a struct 'Compression_Impl' that
// provides implementation for compression and decompression for all supported
// types of compression algorithms.
//

// BMQ

#include <bmqt_compressionalgorithmtype.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>

namespace BloombergLP {

namespace bmqp {

// ==================
// struct Compression
// ==================

/// This struct provides the ability for compression functionality.
struct Compression {
    // CLASS METHODS

    /// Compress the data within the specified `input` as per the specified
    /// `algorithm`, and load the compressed data into the specified
    /// `output`, using the specified `factory` to supply data buffers.
    /// Return 0 on success, and non-zero otherwise.  Also, optionally
    /// specify an `errorStream` to record details on any errors that may
    /// occur during this operation. Finally, as an option specify
    /// `allocator` which will be used to supply memory. Note, that any
    /// existing data in the specified `output` will be preserved.
    static int compress(bdlbb::Blob*                         output,
                        bdlbb::BlobBufferFactory*            factory,
                        bmqt::CompressionAlgorithmType::Enum algorithm,
                        const bdlbb::Blob&                   input,
                        bsl::ostream*                        errorStream = 0,
                        bslma::Allocator*                    allocator   = 0);

    /// Compress the data within the specified `input` as per the specified
    /// `algorithm`, and load the compressed data into the specified
    /// `output`, using the specified `factory` to supply data buffers.
    /// Return 0 on success, and non-zero otherwise.  Also, optionally
    /// specify an `errorStream` to record details on any errors that may
    /// occur during this operation. Finally, as an option specify
    /// `allocator` which will be used to supply memory. Note, that for
    /// specified `algorithm` type `bmqt::CompressionAlgorithmType::e_NONE`,
    /// this returns output Blob with a deep copy of specified `input`
    /// character array. Also note, that any existing data in the specified
    /// `output` will be preserved.
    static int compress(bdlbb::Blob*                         output,
                        bdlbb::BlobBufferFactory*            factory,
                        bmqt::CompressionAlgorithmType::Enum algorithm,
                        const char*                          input,
                        int                                  inputLength,
                        bsl::ostream*                        errorStream = 0,
                        bslma::Allocator*                    allocator   = 0);

    /// Decompress the data within the specified `input` as per the
    /// specified `algorithm`, and load the uncompressed data into specified
    /// `output`, using the specified `factory` to supply the needed data
    /// buffers. Return 0 on success, and non-zero otherwise. Optionally
    /// specify an `errorStream` to record details on any errors that may
    /// occur during this operation. Also, optionally specify `allocator`
    /// which will be used to supply memory.  Also note, that any existing
    /// data in the specified `output` will be preserved.
    static int decompress(bdlbb::Blob*                         output,
                          bdlbb::BlobBufferFactory*            factory,
                          bmqt::CompressionAlgorithmType::Enum algorithm,
                          const bdlbb::Blob&                   input,
                          bsl::ostream*                        errorStream = 0,
                          bslma::Allocator*                    allocator = 0);
};

// ======================
// struct CompressionImpl
// ======================

/// This struct provides implementation for compression and decompression
/// logic for different compression algorithm types.
struct Compression_Impl {
    // CLASS METHODS

    /// Compress the data within the specified `input` as per the Zlib
    /// compression mechanism, and load the compressed data into the
    /// specified `output`, using the specified `factory` to supply data
    /// buffers. Return 0 on success, and non-zero otherwise. Specify
    /// a compression `level`, with 0 indicating non compression, 1
    /// indicating fast compression, and 9 indicating best compression. If
    /// level is -1, the default compression level is used here. Also,
    /// specify an `errorStream` to record details on any errors that may
    /// occur during this operation. Finally, specify `allocator` which will
    /// be used to supply memory. The behavior is undefined unless `level`
    /// is within the range `[-1..9]`. Return 0 on success, and non-zero
    /// otherwise.
    static int compressZlib(bdlbb::Blob*              output,
                            bdlbb::BlobBufferFactory* factory,
                            const bdlbb::Blob&        input,
                            int                       level,
                            bsl::ostream*             errorStream,
                            bslma::Allocator*         allocator);

    /// Decompress the data within the specified `input` as according to the
    /// Zlib algorithm, and load the uncompressed data into the specified
    /// `output` blob, using the specified `factory` to supply needed data
    /// buffers. Return 0 on success, and non-zero otherwise. Specify an
    /// `errorStream` to record details on any errors that may occur during
    /// this operation. Also, specify `allocator` which will be used to
    /// supply memory. Return 0 on success, and non-zero otherwise.
    static int decompressZlib(bdlbb::Blob*              output,
                              bdlbb::BlobBufferFactory* factory,
                              const bdlbb::Blob&        input,
                              bsl::ostream*             errorStream,
                              bslma::Allocator*         allocator);
};

}  // close package namespace
}  // close enterprise namespace

#endif
