// Copyright 2016-2023 Bloomberg Finance L.P.
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

// bmqp_crc32c.h                                                      -*-C++-*-
#ifndef INCLUDED_BMQP_CRC32C
#define INCLUDED_BMQP_CRC32C

//@PURPOSE: Provide utilities to calculate the CRC32-C checksum of a dataset.
//
//@CLASSES:
//  bmqp::Crc32c     : calculates CRC32-C checksum
//  bmqp::Crc32c_Impl: calculates CRC32-C checksum with alternative impl.
//
//@SEE_ALSO: bdlde::crc32
//
//@DESCRIPTION: This component defines a struct, 'bmqp::Crc32c', to calculate a
// CRC32-C checksum (a cyclic redundancy check, comprised of 32 bits, that uses
// the Castagnoli polynomial), using a hardware-accelerated implementation if
// supported or a software implementation otherwise.  It additionally defines
// the struct 'bmqp::Crc32c_Impl' to expose alternative implementations that
// should not be used other than to test and benchmark.  Note that a CRC32-C
// checksum is a strong and fast technique for determining whether or not a
// message was received without errors.  Note, that you also need to check the
// number of bytes in the message, otherwise it is possible to add bytes at the
// end of what is being checked to get the checksums to come out the same.
// Also note that a CRC-32 checksum does not aid in error correction and is not
// naively useful in any sort of cryptography application.
//
/// Thread Safety
///-------------
// Thread safe.
//
/// Support for Hardware Acceleration
///---------------------------------
// Hardware-accelerated implementation is enabled at compile time when building
// on a supported architecture with a compatible compiler.  In addition,
// runtime checks are performed to detect whether the running platform has the
// required hardware support:
//: o x86:   SSE4.2 instructions are required
//: o sparc: runtime check is detected by the 'is_sparc_crc32c_avail' system
//:   call
//
/// Performance
///-----------
// Below are performance comparisons of the hardware-accelerated and software
// implementations against various alternative implementations that compute a
// 32-bit CRC checksum.  They were obtained on a Linux machine with the
// following CPU architecture:
//..
//  $ lscpu
//  Architecture:          x86_64
//  CPU op-mode(s):        32-bit, 64-bit
//  Byte Order:            Little Endian
//  CPU(s):                40
//  On-line CPU(s) list:   0-39
//  Thread(s) per core:    2
//  Core(s) per socket:    10
//  Socket(s):             2
//  NUMA node(s):          2
//  Vendor ID:             GenuineIntel
//  CPU family:            6
//  Model:                 62
//  Stepping:              4
//  CPU MHz:               3001.000
//  BogoMIPS:              5982.81
//  Virtualization:        VT-x
//  L1d cache:             32K
//  L1i cache:             32K
//  L2 cache:              256K
//  L3 cache:              25600K
//  NUMA node0 CPU(s):     0-9,20-29
//  NUMA node1 CPU(s):     10-19,30-39
//..
//
/// Throughput
///  - - - - -
//..
//  Default (Hardware Acceleration)| 20.363  GB per second
//  Software                       | 1.582   GB per second
//  BDE 'bdlde::crc32'             | 374.265 MB per second
//..
//
/// Calculation Time
///  - - - - - - - -
// In the tables below:
//: o !Time! is an average (in absolute nanoseconds) measured over a tight loop
//:   of 100,000 iterations.
//:
//: o !Size! is the size (in bytes) of a 'char *' of random input. Note that it
//:   uses IEC base2 notation (e.g. 1Ki = 2^10 = 1024, 1Mi = 2^20 = 1,048,576).
//
/// 64-bit Default (Hardware Acceleration) vs. BDE's 'bdlde::crc32'
///    -   -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//..
//  ===========================================================================
//  | Size(B) | Def time(ns) | bdlde::crc32 time(ns)| Ratio(bdlde::crc32 / Def)
//  ===========================================================================
//  |       11|             9|                    16|                     1.783
//  |       16|             9|                    28|                     3.089
//  |       21|            10|                    39|                     3.932
//  |       59|            13|                   136|                    10.323
//  |       64|            12|                   150|                    11.675
//  |       69|            13|                   161|                    11.669
//  |      251|            30|                   632|                    20.509
//  |      256|            30|                   640|                    20.660
//  |      261|            37|                   654|                    17.415
//  |     1019|           155|                  2588|                    16.698
//  |     1 Ki|            45|                  2602|                    57.324
//  |     1029|            50|                  2614|                    51.443
//  |     4091|           299|                 10436|                    34.865
//  |     4 Ki|           176|                 10448|                    59.040
//  |     4101|           190|                 10456|                    54.763
//  |    16379|           864|                 41806|                    48.366
//  |    16 Ki|           724|                 41829|                    57.721
//  |    16389|           754|                 40699|                    53.944
//  |    64 Ki|          2858|                162340|                    56.787
//  |   256 Ki|         11925|                654410|                    54.875
//  |     1 Mi|         50937|               2664898|                    52.317
//  |     4 Mi|        198662|              10562189|                    53.167
//  |    16 Mi|        796534|              42570294|                    53.444
//  |    64 Mi|       9976933|             169051561|                    16.944
//..
//
/// 64-bit Software (SW) vs. BDE's 'bdlde::crc32'
///    -   -  -  -  -  -  -  -  -  -  -  -  -  -
//..
//  ==========================================================================
//  | Size(B) | SW time(ns) | bdlde::crc32 time(ns) | Ratio(bdlde::crc32 / SW)
//  ==========================================================================
//  |       11|           13|                     17|                    1.229
//  |       16|           15|                     29|                    1.895
//  |       21|           26|                     39|                    1.500
//  |       59|           44|                    137|                    3.082
//  |       64|           43|                    150|                    3.428
//  |       69|           53|                    161|                    3.017
//  |      251|          158|                    629|                    3.977
//  |      256|          158|                    640|                    4.053
//  |      261|          173|                    654|                    3.777
//  |     1019|          621|                   2592|                    4.170
//  |     1 Ki|          621|                   2602|                    4.185
//  |     1029|          633|                   2614|                    4.128
//  |     4091|         2456|                  10435|                    4.248
//  |     4 Ki|         2457|                  10447|                    4.252
//  |     4101|         2464|                  10462|                    4.246
//  |    16379|         9795|                  41820|                    4.270
//  |    16 Ki|         9798|                  41838|                    4.270
//  |    16389|         9798|                  41846|                    4.271
//  |    64 Ki|        39222|                 167394|                    4.268
//  |   256 Ki|       156894|                 665589|                    4.242
//  |     1 Mi|       629828|                2656343|                    4.218
//  |     4 Mi|      2575623|               10601903|                    4.116
//  |    16 Mi|     10085862|               42775171|                    4.241
//  |    64 Mi|     40705975|              169682572|                    4.168
//..
//
/// Performance (sparc)
///-------------------
// Below are software vs hardware performance comparison for different sparc
// CPUs:
//..
//  SPARC T5: 10.1465 times faster, at 710,138 iterations per second
//  SPARC T7: 10.1007 times faster, at 808,625 iterations per second
//  SPARC T8: 7.66392 times faster, at 1,013,937 iterations per second
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
/// Example 1: Computing and updating a checksum
///  - - - - - - - - - - - - - - - - - - - - - -
// The following code illustrates how to calculate and update a CRC32-C
// checksum for a message over the course of building the full message.
//
// First, let's initialize the utility.
//..
//  bmqp::Crc32c::initialize();
//..
// Now, compute a checksum.
//..
//  // Prepare a message
//  bsl::string message = "This is a test message.";
//
//  // Generate a checksum for 'message'
//  bsl::uint32_t checksum = bmqp::Crc32c::calculate(message.c_str(),
//                                                  message.size());
//..
// Finally, if we learn that our message has grown by another chunk and we want
// to compute the checksum of the original message plus the new chunk, let's
// update the checksum by using it as a starting point.
//..
//  // New chunk
//  bsl::string newChunk = "This is a chunk appended to original message";
//  message += newChunk;
//
//  // Update checksum using previous value as starting point
//  checksum = bmqp::Crc32c::calculate(newChunk.c_str(),
//                                     newChunk.size(),
//                                     checksum);
//..
//

// BMQ

// BDE
#include <bdlbb_blob.h>
#include <bsl_cstdint.h>

namespace BloombergLP {
namespace bmqp {

// =============
// struct Crc32c
// =============

/// This class provides runtime-efficient utilities to calculate a CRC32-C
/// checksum.
struct Crc32c {
  public:
    // TYPES

    /// Signature of the function for the calculation of CRC32-C in the
    /// default implementation methods (`calculate`) .
    typedef bsl::uint32_t (*Crc32cFn)(const bsl::uint8_t* data,
                                      bsl::uint32_t       length,
                                      bsl::uint32_t       crc);

    // CONSTANTS

    /// CRC32-C value for a 0 length input.  Note that a buffer with this
    /// CRC32-C value need not be a 0 length input.
    static const bsl::uint32_t k_NULL_CRC32C = 0U;

    // CLASS METHODS

    /// Initialize the utilities with a platform-dependent mechanism to
    /// compute crc32c checksums.  This method only needs to be called once
    /// before any other method, but can be called multiple times.
    static void initialize();

    /// Return the CRC32-C value calculated for the specified `data` over
    /// the specified `length` number of bytes, using the optionally
    /// specified `crc` value as the starting point for the calculation.
    /// This utilizes the default implementation as set by `initialize()`.
    /// The behavior is undefined unless `initialize()` has been called
    /// prior to calling this method at least once.  Note that if `data` is
    /// 0, then `length` also must be 0.
    static bsl::uint32_t calculate(const void*   data,
                                   bsl::uint32_t length,
                                   bsl::uint32_t crc = k_NULL_CRC32C);

    /// Return the CRC32-C value calculated over the buffers in the
    /// specified `blob` (in order of `blob.buffer(idx)` for increasing
    /// values of `idx`), using the optionally specified `crc` value as the
    /// starting point for the calculation.  This utilizes the default
    /// implementation as set by `initialize()`.  The behavior is undefined
    /// unless `initialize()` has been called prior to calling this method
    /// at least once.
    static bsl::uint32_t calculate(const bdlbb::Blob& blob,
                                   bsl::uint32_t      crc = k_NULL_CRC32C);
};

// ==================
// struct Crc32c_Impl
// ==================

/// This class provides alternative implementations of utilities to
/// calculate a CRC32-C checksum.
struct Crc32c_Impl {
  public:
    // CLASS METHODS

    /// Return the CRC32-C value calculated for the specified `data` over
    /// the specified `length` number of bytes, using the optionally
    /// specified `crc` value as the starting point for the calculation.
    /// This utilizes a portable software-based implementation to perform
    /// the calculation.  Note that if `data` is 0, then `length` must also
    /// be 0.
    static bsl::uint32_t
    calculateSoftware(const void*   data,
                      bsl::uint32_t length,
                      bsl::uint32_t crc = Crc32c::k_NULL_CRC32C);

    /// Return the CRC32-C value calculated over all the buffers in the
    /// specified `blob` (in order of `blob.buffer(idx)` for increasing
    /// values of `idx`), using the optionally specified `crc` value as the
    /// starting point for the calculation.  This utilizes a portable
    /// software-based implementation to perform the calculation.
    static bsl::uint32_t
    calculateSoftware(const bdlbb::Blob& blob,
                      bsl::uint32_t      crc = Crc32c::k_NULL_CRC32C);

    /// Return the CRC32-C value calculated for the specified `data` over
    /// the specified `length` number of bytes, using the optionally
    /// specified `crc` value as the starting point for the calculation.
    /// This utilizes a hardware-based implementation that does not leverage
    /// instruction level parallelism to perform the calculation (hence it
    /// calculates the crc32c in "serial").  Note that this function will
    /// fall back to the software version when running on unsupported
    /// platforms.  Also note that if `data` is 0, then `length` must also
    /// be 0.
    static bsl::uint32_t
    calculateHardwareSerial(const void*   data,
                            bsl::uint32_t length,
                            bsl::uint32_t crc = Crc32c::k_NULL_CRC32C);
};

}  // close package namespace
}  // close enterprise namespace

#endif
