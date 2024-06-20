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

// mqbs_filestoreprotocol.h                                           -*-C++-*-
#ifndef INCLUDED_MQBS_FILESTOREPROTOCOL
#define INCLUDED_MQBS_FILESTOREPROTOCOL

//@PURPOSE: Provide definitions for BlazingMQ file storage protocol structures.
//
//@CLASSES:
//  mqbs::FileStoreProtocol:
//  mqbs::FileType:
//  mqbs::FileHeader:
//  mqbs::DataFileHeader:
//  mqbs::JournalFileHeader:
//  mqbs::QlistFileHeader:
//  mqbs::DataHeader:
//  mqbs::QueueRecordHeader:
//  mqbs::RecordType:
//  mqbs::RecordHeader:
//  mqbs::MessageRecord:
//  mqbs::ConfirmReason:
//  mqbs::ConfirmRecord:
//  mqbs::DeletionRecordFlag:
//  mqbs::DeletionRecord:
//  mqbs::QueueOpType:
//  mqbs::QueueOpRecord:
//  mqbs::JournalOpType:
//  mqbs::JournalOpRecord:
//
//@SEE ALSO: mqbs_filestorage.h
//
//@DESCRIPTION: 'mqbs::FileStoreProtocol' provide definitions for a set of
// structures defining the binary layout of the protocol messages used by BMQ
// file storage to persist messages on disk.  See
// doc/proposal/file_backed_storage.md for more details.
//
/// Protocol Limitations
///--------------------
//..
//     =======================================================================
//     + Internal:
//       Max concurrent protocols.......................: 4
//       Max number of file types.......................: 127
//       Max FileHeader size............................: 252 bytes
//       Max DataFileHeader size........................: 1020 bytes
//       Max JournalFileHeader size.....................: 1020 bytes
//       Max QlistFileHeader size.......................: 1020 bytes
//       Max QueueRecordHeader size.....................: 1020 bytes
//       Max Data file size.............................: 32GB
//                               (courtesy MessageRecord.d_messageOffsetDwords)
//       Max Journal file size..........................: 16GB
//                          (courtesy bmqp::StorageHeader.d_journalOffsetWords)
//       Max Qlist file size............................: 16GB
//                         (courtesy QueueOpRecord.d_queueUriRecordOffsetWords)
//       Max Journal Record size........................: 1020 bytes
//       Max Journal Record types.......................: 15
//
//     -----------------------------------------------------------------------
//     + External:
//       Max queue URI length...........................: ~256 KB
//       Max app ID length..............................: ~256 KB
//       Max payload length.............................: ~4 GB
//                                           (courtesy DataHeader.d_dataLength)
//       Max message reference count....................: 4096
//     ========================================================================
//..
//
/// Message Layout in DATA and JOURNAL Files
///----------------------------------------
//..
//        JOURNAL                             DATA
//      -----------                    -------------------
//      :         :                    :                 :
//      :         :                    :                 :
//      :.........:                    :                 :
//      :         :                    :                 :
//      :         :                    :                 :
//      :_________:                    :                 :
//      | CONFIRM |                    :                 :
//      | RECORD  |                    :_________________:
//      |_________|       .----------->| mqbs::DataHeader|
//      |DELETION |       |            |    (8 bytes)    |
//      | RECORD  |       |            |_________________|
//      |_________|       |            |                 |
//      | MESSAGE |       |            | Options, if any |
//      | RECORD  |-------'            | (variable size) |
//      |_________|                    |_________________|
//      |         |                    |                 |
//      |         |                    | Application Data|
//      |_________|                    |(msg properties, |
//      |         |                    |if any, and msg  |
//      |         |                    |payload; variable|
//      |_________|                    |size)            |
//      :         :                    |                 |
//      :         :                    |                 |
//      :.........:                    |_________________|
//      :         :                    :                 :
//      :         :                    :                 :
//..

// MQB

#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>  // for bmqp::Protocol::k_WORD_SIZE, etc.
#include <bmqt_compressionalgorithmtype.h>

#include <bmqt_messageguid.h>

// BDE
#include <bdlb_bigendian.h>
#include <bsl_cstddef.h>
#include <bsl_cstring.h>  // for bsl::memset & bsl::memcpy
#include <bsl_ostream.h>
#include <bsls_types.h>

#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbs {

// ========================
// struct FileStoreProtocol
// ========================

/// Namespace for protocol generic values and routines
struct FileStoreProtocol {
    // CONSTANTS
    static const int k_VERSION = 1;
    // Version of the protocol

    static const int k_HASH_LENGTH = 16;

    static const int k_KEY_LENGTH = mqbu::StorageKey::e_KEY_LENGTH_BINARY;
    // How many first few bytes of hash are used in keys

    static const int k_JOURNAL_RECORD_SIZE = 60;
    // Record size

    static const int k_MAX_MSG_REF_COUNT_HARD = 4096;
    // Maximum value of reference count for a given message
    // per BlazingMQ file store protocol

    static const int k_NUM_FILES_PER_PARTITION = 3;
    // Number of files per partition (data, journal & qlist)

    static const char* k_DATA_FILE_EXTENSION;

    static const char* k_JOURNAL_FILE_EXTENSION;

    static const char* k_QLIST_FILE_EXTENSION;

    static const char* k_COMMON_FILE_EXTENSION_PREFIX;

    static const char* k_COMMON_FILE_PREFIX;
};

// ==============
// struct Bitness
// ==============

/// This struct defines bitness of the task which wrote BlazingMQ file
/// store.
struct Bitness {
    // TYPES
    enum Enum { e_32 = 1, e_64 = 2 };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a `FileType::Enum`
    /// value.
    static bsl::ostream& print(bsl::ostream& stream,
                               Bitness::Enum value,
                               int           level          = 0,
                               int           spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << Bitness::toAscii(Bitness::e_64);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// 64
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(Bitness::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, Bitness::Enum value);

// ===============
// struct FileType
// ===============

/// This struct defines the type of files present in file-backed storage in
/// BlazingMQ broker.
struct FileType {
    // TYPES
    enum Enum { e_UNDEFINED = 0, e_DATA = 1, e_JOURNAL = 2, e_QLIST = 3 };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a `FileType::Enum`
    /// value.
    static bsl::ostream& print(bsl::ostream&  stream,
                               FileType::Enum value,
                               int            level          = 0,
                               int            spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << FileType::toAscii(FileType::e_JOURNAL);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// JOURNAL
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(FileType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, FileType::Enum value);

// =================
// struct FileHeader
// =================

/// This struct represents the header for all types of files maintained by a
/// BlazingMQ file store.  This header is also referred to as the "BlazingMQ
/// header".
struct FileHeader {
    // FileHeader structure datagram [32 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |                           Magic1                              |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Magic2                              |
    //   +---------------+---------------+---------------+---------------+
    //   |PV |   HW      |B|  FileType   |            Reserved           |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                         PartitionId                           |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //
    //  Magic1.................: First magic word
    //  Magic2.................: Second magic word
    //  Protocol Version (PV)..: Protocol Version (up to 4 concurrent versions)
    //  Header Words (HW)......: Number of words in this file header
    //  Bitness (B)............: Bitness of task writing this file
    //  FileType...............: Type of BlazingMQ file
    //  PartitionId............: This file's partitionId
    //..

    // NOTE: Size of this struct must be a multiple of 8 due to the 8-byte
    //       alignment requirement enforced by the BlazingMQ DATA file.

  private:
    // PRIVATE CONSTANTS
    static const int k_PROTOCOL_VERSION_NUM_BITS = 2;
    static const int k_HEADER_WORDS_NUM_BITS     = 6;
    static const int k_FILE_TYPE_NUM_BITS        = 7;
    static const int k_BITNESS_NUM_BITS          = 1;

    static const int k_PROTOCOL_VERSION_START_IDX = 6;
    static const int k_HEADER_WORDS_START_IDX     = 0;
    static const int k_FILE_TYPE_START_IDX        = 0;
    static const int k_BITNESS_START_IDX          = 7;

    static const int k_PROTOCOL_VERSION_MASK;
    static const int k_HEADER_WORDS_MASK;
    static const int k_FILE_TYPE_MASK;
    static const int k_BITNESS_MASK;

  public:
    // CONSTANTS
    static const int          k_MIN_HEADER_SIZE = 9;
    static const unsigned int k_MAGIC1          = 0x21626D71;  // !bmq
    static const unsigned int k_MAGIC2          = 0x424D5121;  // BMQ!

  private:
    // DATA
    bdlb::BigEndianUint32 d_magic1;

    bdlb::BigEndianUint32 d_magic2;

    unsigned char d_protoVerAndHeaderWords;

    char d_bitnessAndFileType;

    char d_reserved1[10];

    bdlb::BigEndianInt32 d_partitionId;

    char d_reserved2[8];

  public:
    // CREATORS

    /// Create this object with its type set to `FileType::e_UNDEFINED`,
    /// `headerWords` field set to appropriate value derived from sizeof()
    /// operator, protocol version set to the current one, bitness and magic
    /// fields set to appropriate value.  All other fields are set to zero.
    FileHeader();

    // MANIPULATORS
    FileHeader& setMagic1(unsigned int value);

    FileHeader& setMagic2(unsigned int value);

    FileHeader& setProtocolVersion(unsigned char value);

    FileHeader& setHeaderWords(unsigned char value);

    FileHeader& setBitness(Bitness::Enum value);

    FileHeader& setFileType(FileType::Enum value);

    FileHeader& setPartitionId(int value);

    // ACCESSORS
    unsigned int magic1() const;

    unsigned int magic2() const;

    unsigned char protocolVersion() const;

    Bitness::Enum bitness() const;

    FileType::Enum fileType() const;

    unsigned char headerWords() const;

    int partitionId() const;
};

// =====================
// struct DataFileHeader
// =====================

/// This struct represents the header for `DATA` file of a BlazingMQ file
/// store.
struct DataFileHeader {
    // DataFileHeader structure datagram [8 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |  HeaderWords  |    Reserved   |           FileKey             |
    //   +---------------+---------------+---------------+---------------+
    //   |                    FileKey                    |    Reserved   |
    //   +---------------+---------------+---------------+---------------+
    //
    //  HeaderWords..: Number of words in this struct
    //  FileKey......: First 5 bytes of hashed file name
    //..
    //

    // NOTE: Size of this struct must be a multiple of 8 due to the 8-byte
    //       alignment requirement enforced by the BlazingMQ DATA file.

  public:
    // CONSTANTS
    static const int k_MIN_HEADER_SIZE = 1;

  private:
    // DATA
    unsigned char d_headerWords;

    char d_reserved1;

    char d_fileKey[FileStoreProtocol::k_KEY_LENGTH];

    char d_reserved2;

  public:
    // CREATORS

    /// Create this object with `headerWords` field set to appropriate value
    /// derived from sizeof() operator.  All other fields are set to zero.
    DataFileHeader();

    // MANIPULATORS
    DataFileHeader& setHeaderWords(unsigned char value);

    DataFileHeader& setFileKey(const mqbu::StorageKey& value);

    // ACCESSORS
    unsigned char headerWords() const;

    const mqbu::StorageKey& fileKey() const;
};

// ========================
// struct JournalFileHeader
// ========================

/// This struct represents the header for `JOURNAL` file of a BlazingMQ file
/// store.
struct JournalFileHeader {
    // JournalFileHeader structure datagram [12 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |  HeaderWords  | RecordWords   |           Reserved            |
    //   +---------------+---------------+---------------+---------------+
    //   |            First SyncPoint Record Offset Upper Bits           |
    //   +---------------+---------------+---------------+---------------+
    //   |            First SyncPoint Record Offset Lower Bits           |
    //   +---------------+---------------+---------------+---------------+
    //
    //  HeaderWords...................: Number of words in this struct
    //  RecordWords...................: Number of words in a journal record
    //  First Syncoint Record Offset..: Offset of first sync point record in
    //                                  the journal
    //..
    //

  private:
    // PRIVATE CONSTANTS
    static const int k_NUM_RESERVED_BYTES = 2;

  public:
    // CONSTANTS
    static const int k_MIN_HEADER_SIZE = 1;

  private:
    // DATA
    unsigned char d_headerWords;

    unsigned char d_recordWords;

    char d_reserved[k_NUM_RESERVED_BYTES];

    bdlb::BigEndianUint32 d_firstSyncPointOffsetUpperBits;

    bdlb::BigEndianUint32 d_firstSyncPointOffsetLowerBits;
    // 0 == null

  public:
    // CREATORS

    /// Create this object with `headerWords` and `recordWords` fields set
    /// to appropriate values derived from sizeof() operator, and
    /// FileStoreProtocol::k_JOURNAL_RECORD_SIZE respectively.  All other
    /// fields are set to zero.
    JournalFileHeader();

    // MANIPULATORS
    JournalFileHeader& setHeaderWords(unsigned char value);

    JournalFileHeader& setRecordWords(unsigned char value);

    JournalFileHeader& setFirstSyncPointOffsetWords(bsls::Types::Uint64 value);

    // ACCESSORS
    unsigned char headerWords() const;

    unsigned char recordWords() const;

    bsls::Types::Uint64 firstSyncPointOffsetWords() const;
};

// ======================
// struct QlistFileHeader
// ======================

/// This struct represents the header for `QLIST` file of a BlazingMQ file
/// store.
struct QlistFileHeader {
    // QlistFileHeader structure datagram [4 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |      HW       |                  Reserved                     |
    //   +---------------+---------------+---------------+---------------+
    //
    //  HeaderWords (HW)................: Number of words in this struct
    //..
    //

  public:
    // CONSTANTS
    static const int k_MIN_HEADER_SIZE = 1;

  private:
    // PRIVATE CONSTANTS
    static const int k_NUM_RESERVED_BYTES = 3;

  private:
    // DATA
    unsigned char d_headerWords;

    char d_reserved[k_NUM_RESERVED_BYTES];

  public:
    // CREATORS

    /// Create this object with `headerWords` field set to appropriate
    /// value derived from sizeof() operator.  All other fields are set to
    /// zero.
    QlistFileHeader();

    // MANIPULATORS
    QlistFileHeader& setHeaderWords(unsigned char value);

    // ACCESSORS
    unsigned char headerWords() const;
};

// ======================
// struct DataHeaderFlags
// ======================

/// This struct defines the meanings of each bits of the flags field of the
/// `DataHeader` structure.
struct DataHeaderFlags {
    // TYPES
    enum Enum {
        e_MESSAGE_PROPERTIES = (1 << 0)  // Contains message properties
        ,
        e_UNUSED2 = (1 << 1),
        e_UNUSED3 = (1 << 2),
        e_UNUSED4 = (1 << 3),
        e_UNUSED5 = (1 << 4),
        e_UNUSED6 = (1 << 5),
        e_UNUSED7 = (1 << 6),
        e_UNUSED8 = (1 << 7)
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).
    static bsl::ostream& print(bsl::ostream&         stream,
                               DataHeaderFlags::Enum value,
                               int                   level          = 0,
                               int                   spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(DataHeaderFlags::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(DataHeaderFlags::Enum*   out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, DataHeaderFlags::Enum value);

// =========================
// struct DataHeaderFlagUtil
// =========================

/// This component provides utility methods for `bmqp::DataHeaderFlags`.
struct DataHeaderFlagUtil {
    // CLASS METHODS

    /// Set the specified `flag` to true in the bit-mask in the specified
    /// `flags`.
    static void setFlag(int* flags, DataHeaderFlags::Enum flag);
    static void unsetFlag(int* flags, DataHeaderFlags::Enum flag);

    /// Return true if the bit-mask in the specified `flags` has the
    /// specified `flag` set, or false if not.
    static bool isSet(int flags, DataHeaderFlags::Enum flag);

    /// Check whether the specified `flags` represent a valid combination of
    /// flags to use for opening a queue.  Return true if it does, or false
    /// if some exclusive flags are both set, and populate the specified
    /// `errorDescription` with the reason of the failure.
    static bool isValid(bsl::ostream& errorDescription, int flags);

    /// Print the ascii-representation of all the values set in the
    /// specified `flags` to the specified `stream`.  Each value is `,`
    /// separated.
    static bsl::ostream& prettyPrint(bsl::ostream& stream, int flags);

    /// Convert the string representation of the enum bit mask from the
    /// specified `str` (which format corresponds to the one of the
    /// `prettyPrint` method) and populate the specified `out` with the
    /// result on success returning 0, or return a non-zero error code on
    /// error, populating the specified `errorDescription` with a
    /// description of the error.
    static int fromString(bsl::ostream&      errorDescription,
                          int*               out,
                          const bsl::string& str);
};

// =================
// struct DataHeader
// =================

/// This struct represents the header for each message present in the `DATA`
/// file of a BlazingMQ file store.
struct DataHeader {
    // DataHeader structure datagram [4 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |  HW |                     MessageWords                        |
    //   +---------------+---------------+---------------+---------------+
    //   |                 OptionsWords                  |    Flags      |
    //   +---------------+---------------+---------------+---------------+
    //   |           SchemaId            |           Reserved            |
    //   +---------------+---------------+---------------+---------------+
    //      HW..: HeaderWords
    //
    //  HeaderWords (HW)..: Total size (in words) of this DataHeader
    //  MessageWords......: Total size (in words) of the message, including
    //                      this header, any sub-headers, message properties,
    //                      data payload, and padding
    //  OptionsWords......: Total size (in words) of the options area
    //  Flags.............: Flags; see DataHeaderFlags struct.
    //..

  public:
    // CONSTANTS
    static const int k_MIN_HEADER_SIZE = 4;

    /// Minimum size (bytes) of a `DataHeader` that contains SchemaId.
    static const int k_MIN_HEADER_SIZE_FOR_SCHEMA_ID = 12;

    // PUBLIC TYPES
    typedef struct DataHeaderFlags    Flags;
    typedef struct DataHeaderFlagUtil FlagUtil;

  private:
    // PRIVATE CONSTANTS
    static const int k_HEADER_WORDS_NUM_BITS = 3;
    static const int k_MSG_WORDS_NUM_BITS    = 29;

    static const int k_HEADER_WORDS_START_IDX = 29;
    static const int k_MSG_WORDS_START_IDX    = 0;

    static const int k_HEADER_WORDS_MASK;
    static const int k_MSG_WORDS_MASK;

    static const int k_OPTIONS_WORDS_NUM_BITS = 24;
    static const int k_FLAGS_NUM_BITS         = 8;

    static const int k_OPTIONS_WORDS_START_IDX = 8;
    static const int k_FLAGS_START_IDX         = 0;

    static const int k_OPTIONS_WORDS_MASK;
    static const int k_FLAGS_MASK;

  private:
    // DATA
    bdlb::BigEndianUint32 d_headerWordsAndMessageWords;

    bdlb::BigEndianUint32 d_optionsWordsAndFlags;

    bmqp::SchemaWireId d_schemaId;

    unsigned char d_reserved[2];
    // Reserved.
  public:
    // CREATORS

    /// Create an instance where `headerWords` and `messageWords` fields
    /// are set to size of the object, the value for which is directly
    /// derived from the `sizeof` operator.  All other fields are set to
    /// zero.
    DataHeader();

    // MANIPULATORS
    DataHeader& setHeaderWords(int value);

    DataHeader& setMessageWords(int value);

    DataHeader& setOptionsWords(int value);

    DataHeader& setFlags(int value);

    bmqp::SchemaWireId& schemaId();

    // ACCESSORS
    int headerWords() const;

    int messageWords() const;

    int optionsWords() const;

    int flags() const;

    const bmqp::SchemaWireId& schemaId() const;
};

// ==================
// struct AppIdHeader
// ==================

/// This struct represents the header for each appId present in a QueueUri
/// record in QLIST file of a BlazingMQ file store.
struct AppIdHeader {
    // AppIdHeader structure datagram [4 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |           Reserved            |       AppId Length Words      |
    //   +---------------+---------------+---------------+---------------+
    //
    //  AppId Length Words.....: Length (in words) of the appId including
    //                           padding
    //..
    //
    // See 'struct QueueRecordHeader' to see the usage of this header struct.

  public:
    // CONSTANTS
    static const int k_MIN_HEADER_SIZE = 4;

  private:
    // PRIVATE CONSTANTS
    static const int k_RESERVED_FIELD_NUM_BYTES = 12;

  private:
    // DATA
    bdlb::BigEndianUint16 d_reserved;

    bdlb::BigEndianUint16 d_appIdLengthWords;

  public:
    // CREATORS

    /// Create this object with all fields set to zero.
    AppIdHeader();

    // MANIPULATORS
    AppIdHeader& setAppIdLengthWords(unsigned int value);

    // ACCESSORS
    unsigned int appIdLengthWords() const;
};

// ========================
// struct QueueRecordHeader
// ========================

/// This struct represents the header for each record present in the `QLIST`
/// file of a BlazingMQ file store.
struct QueueRecordHeader {
    // QueueRecordHeader structure datagram [20 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |    QueueUri Length Words      |           Num AppIds          |
    //   +---------------+---------------+---------------+---------------+
    //   |  HeaderWords  |             QueueRecordWords                  |
    //   +---------------+---------------+---------------+---------------+
    //   |                            Reserved                           |
    //   +---------------+---------------+---------------+---------------+
    //   |                            Reserved                           |
    //   +---------------+---------------+---------------+---------------+
    //   |                            Reserved                           |
    //   +---------------+---------------+---------------+---------------+
    //
    //  QueueUri Length Words..: Length (in words) of the queue uri including
    //                           padding
    //  Num AppIds.............: Total number of appIds present in this record
    //  HeaderWords............: Size (in words) of this header
    //  QueueRecordWords.......: Size (in words) of entire queue record
    //                           including this header struct
    //..
    //
    // A queue record will be laid out like so:
    //..
    //  [QueueRecordHeader][Padded QueueUri][QueueUri Hash][AppIdHeader]
    //  [Padded AppId #1][AppId #1 Hash][AppIdHeader[Padded AppId #2]
    // [AppId #2 Hash]...[AppIdHeader][Padded AppId #n][AppId #n Hash][Magic
    // word]
    //..
    //
    // Note that zero or more AppIds can be present in a queue uri record.
    // Also note that a queue uri record for same queue uri can be present
    // multiple times in the QLIST file, as long as the list of AppIds are
    // different.  Also note that an entire queue record ends with a magic
    // word, as shown in the sample record layout above.

  public:
    // CONSTANTS
    static const int k_MIN_HEADER_SIZE = 8;

    static const unsigned int k_MAGIC = 0x71557249;  // qUrI

  private:
    // PRIVATE CONSTANTS
    static const int k_HEADER_WORDS_NUM_BITS       = 8;
    static const int k_QUEUE_RECORD_WORDS_NUM_BITS = 24;

    static const int k_HEADER_WORDS_START_IDX       = 24;
    static const int k_QUEUE_RECORD_WORDS_START_IDX = 0;

    static const int k_HEADER_WORDS_MASK;
    static const int k_QUEUE_RECORD_WORDS_MASK;

    static const int k_RESERVED_FIELD_NUM_BYTES = 12;

  private:
    // DATA
    bdlb::BigEndianUint16 d_queueUriLengthWords;

    bdlb::BigEndianUint16 d_numAppIds;

    bdlb::BigEndianUint32 d_headerWordsAndQueueRecordWords;

    char d_reserved[k_RESERVED_FIELD_NUM_BYTES];
    // Possible time stamp value

  public:
    // CREATORS

    /// Create this object with all fields set to zero, except for
    /// `headerWords` field, the value of which is set to that returned by
    /// sizeof() operator.
    QueueRecordHeader();

    // MANIPULATORS
    QueueRecordHeader& setQueueUriLengthWords(unsigned int value);

    QueueRecordHeader& setNumAppIds(unsigned int value);

    QueueRecordHeader& setHeaderWords(unsigned int value);

    QueueRecordHeader& setQueueRecordWords(unsigned int value);

    // ACCESSORS
    unsigned int queueUriLengthWords() const;

    unsigned int numAppIds() const;

    unsigned int headerWords() const;

    unsigned int queueRecordWords() const;
};

// =================
// struct RecordType
// =================

/// This struct defines the type of records in the journal file.
struct RecordType {
    // TYPES
    enum Enum {
        e_UNDEFINED  = 0,
        e_MESSAGE    = 1,
        e_CONFIRM    = 2,
        e_DELETION   = 3,
        e_QUEUE_OP   = 4,
        e_JOURNAL_OP = 5
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a `RecordType::Enum`
    /// value.
    static bsl::ostream& print(bsl::ostream&    stream,
                               RecordType::Enum value,
                               int              level          = 0,
                               int              spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << RecordType::toAscii(RecordType::e_DELETION)
    /// ```
    /// will print the following on standard output:
    /// ```
    /// DELETION
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(RecordType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, RecordType::Enum value);

// ===================
// struct RecordHeader
// ===================

/// This struct represents the header for each record present in the
/// `JOURNAL` file of a BlazingMQ file store.
struct RecordHeader {
    // RecordHeader structure datagram [20 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   | Type  |      Flags            |  Sequence Number Upper Bits   |
    //   +---------------+---------------+---------------+---------------+
    //   |                   Sequence Number Lower Bits                  |
    //   +---------------+---------------+---------------+---------------+
    //   |                      Primary Lease Id                         |
    //   +---------------+---------------+---------------+---------------+
    //   |                     Timestamp Upper Bits                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                     Timestamp Lower Bits                      |
    //   +---------------+---------------+---------------+---------------+
    //
    //  Type........................: Type of record
    //  Flags.......................: Record-specific flags or fields.
    //  Primary Lease Id............: LeaseId of the primary node.
    //  Sequence Number Upper Bits..: Upper 16 bits of sequence number.
    //  Sequence Number Lower Bits..: Lower 32 bits of sequence number.
    //  Timestamp Upper Bits........: Upper 32 bits of timestamp
    //  Timestamp Lower Bits........: Lower 32 bits of timestamp
    //..
    //
    // Note that sequence number field has been given a total of 48 bits, which
    // can approximately support a value of 280 trillion.  Also note that
    // 'flags' field has different meaning depending upon the type of record.

  private:
    // PRIVATE CONSTANTS
    static const int k_TYPE_NUM_BITS  = 4;
    static const int k_FLAGS_NUM_BITS = 12;

    static const int k_TYPE_START_IDX  = 12;
    static const int k_FLAGS_START_IDX = 0;

    static const int k_TYPE_MASK;
    static const int k_FLAGS_MASK;

  public:
    // CONSTANTS
    static const unsigned int k_MAGIC = 0x2A724563;  // *rEc

  private:
    // DATA
    bdlb::BigEndianUint16 d_typeAndFlags;

    bdlb::BigEndianUint16 d_seqNumUpperBits;

    bdlb::BigEndianUint32 d_seqNumLowerBits;

    bdlb::BigEndianUint32 d_primaryLeaseId;

    bdlb::BigEndianUint32 d_timestampUpperBits;

    bdlb::BigEndianUint32 d_timestampLowerBits;

  public:
    // CREATORS
    RecordHeader();

    // MANIPULATORS
    RecordHeader& setType(RecordType::Enum value);

    RecordHeader& setFlags(unsigned int value);

    RecordHeader& setPrimaryLeaseId(unsigned int value);

    RecordHeader& setSequenceNumber(bsls::Types::Uint64 value);

    RecordHeader& setTimestamp(bsls::Types::Uint64 value);

    // ACCESSORS
    RecordType::Enum type() const;

    unsigned int flags() const;

    unsigned int primaryLeaseId() const;

    bsls::Types::Uint64 sequenceNumber() const;

    bsls::Types::Uint64 timestamp() const;

    bmqp_ctrlmsg::PartitionSequenceNumber partitionSequenceNumber() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const RecordHeader& rhs);

// ====================
// struct MessageRecord
// ====================

/// This struct represents a MessageRecord present in the `JOURNAL` file of
/// a BlazingMQ file store.
struct MessageRecord {
    // MessageRecord structure datagram [60 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |           Reserved      | CAT |          QueueKey             |
    //   +---------------+---------------+---------------+---------------+
    //   |                  QueueKey                     |    FileKey    |
    //   +---------------+---------------+---------------+---------------+
    //   |                            FileKey                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageOffsetDwords                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                            CRC-32C                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Magic                             |
    //   +---------------+---------------+---------------+---------------+
    //
    //  Header................: Record header
    //  CAT...................: Compression Algorithm Type
    //  QueueKey..............: Queue key to which this message record belongs
    //  FileKey...............: File key of the corresponding data file
    //  MessageOffsetDwords...: Offset (in DWORDS) in the corresponding data
    //                          file of the message
    //  GUID..................: Message GUID
    //  CRC-32C...............: CRC-32C checksum of the message
    //  Magic.................: Magic word
    //..
    //
    // Note that reference count of the message is stored in the 'flags' field
    // of 'RecordHeader'.

  private:
    // PRIVATE CONSTANTS
    static const int  k_CAT_START_IDX = 0;
    static const char k_CAT_MASK;

    // DATA
    RecordHeader d_header;

    char d_reserved;

    char d_reservedAndCAT;

    unsigned char d_queueKey[FileStoreProtocol::k_KEY_LENGTH];

    char d_fileKey[FileStoreProtocol::k_KEY_LENGTH];

    bdlb::BigEndianUint32 d_messageOffsetDwords;

    unsigned char d_guid[bmqt::MessageGUID::e_SIZE_BINARY];

    bdlb::BigEndianUint32 d_crc32c;

    bdlb::BigEndianUint32 d_magic;

  public:
    // CREATORS

    /// Create an instance with all fields unset.
    MessageRecord();

    // MANIPULATORS
    RecordHeader& header();

    MessageRecord& setRefCount(unsigned int value);

    MessageRecord& setCompressionAlgorithmType(
        bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType);

    MessageRecord& setQueueKey(const mqbu::StorageKey& key);

    MessageRecord& setFileKey(const mqbu::StorageKey& key);

    MessageRecord& setMessageOffsetDwords(unsigned int value);

    MessageRecord& setMessageGUID(const bmqt::MessageGUID& value);

    MessageRecord& setCrc32c(unsigned int value);

    MessageRecord& setMagic(unsigned int value);

    // ACCESSORS
    const RecordHeader& header() const;

    unsigned int refCount() const;

    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType() const;

    const mqbu::StorageKey& queueKey() const;

    const mqbu::StorageKey& fileKey() const;

    unsigned int messageOffsetDwords() const;

    const bmqt::MessageGUID& messageGUID() const;

    unsigned int crc32c() const;

    unsigned int magic() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const MessageRecord& rhs);

// ====================
// struct ConfirmReason
// ====================

/// This struct defines the flags applicable to ConfirmRecord, more
/// precisely the reason for confirming the message.
struct ConfirmReason {
    // TYPES
    // This is a bitmask, not enumeration
    enum Enum { e_CONFIRMED = 0, e_REJECTED = 1, e_AUTO_CONFIRMED = 2 };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// ConfirmReason::Enum' value.
    static bsl::ostream& print(bsl::ostream&       stream,
                               ConfirmReason::Enum value,
                               int                 level          = 0,
                               int                 spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << ConfirmReason::toAscii(ConfirmReason::e_CONFIRMED);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// CONFIRMED
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ConfirmReason::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, ConfirmReason::Enum value);

// ====================
// struct ConfirmRecord
// ====================

/// This struct represents a ConfirmRecord present in the `JOURNAL` file of
/// a BlazingMQ file store.
struct ConfirmRecord {
    // ConfirmRecord structure datagram [60 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |           Reserved            |          QueueKey             |
    //   +---------------+---------------+---------------+---------------+
    //   |                  QueueKey                     |    AppKey     |
    //   +---------------+---------------+---------------+---------------+
    //   |                            AppKey                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Magic                             |
    //   +---------------+---------------+---------------+---------------+
    //
    //  Header................: Record header
    //  QueueKey..............: Queue key to which this message record belongs
    //  AppKey................: App key to which this message record belongs
    //  GUID..................: Message GUID
    //  Magic.................: Magic word
    //..
    //
    // Note that ConfirmReason is stored in the 'flags' field of the
    // 'RecordHeader'.

  private:
    // DATA
    RecordHeader d_header;

    char d_reserved1[2];

    char d_queueKey[FileStoreProtocol::k_KEY_LENGTH];

    char d_appKey[FileStoreProtocol::k_KEY_LENGTH];

    unsigned char d_guid[bmqt::MessageGUID::e_SIZE_BINARY];

    char d_reserved2[8];

    bdlb::BigEndianUint32 d_magic;

  public:
    // CREATORS

    /// Create an instance with all fields unset.
    ConfirmRecord();

    // MANIPULATORS
    RecordHeader& header();

    ConfirmRecord& setReason(ConfirmReason::Enum value);

    ConfirmRecord& setQueueKey(const mqbu::StorageKey& key);

    ConfirmRecord& setAppKey(const mqbu::StorageKey& key);

    ConfirmRecord& setMessageGUID(const bmqt::MessageGUID& value);

    ConfirmRecord& setMagic(unsigned int value);

    // ACCESSORS
    const RecordHeader& header() const;

    ConfirmReason::Enum reason() const;

    const mqbu::StorageKey& queueKey() const;

    const mqbu::StorageKey& appKey() const;

    const bmqt::MessageGUID& messageGUID() const;

    unsigned int magic() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const ConfirmRecord& rhs);

// =========================
// struct DeletionRecordFlag
// =========================

/// This struct defines the flags applicable to DeletionRecord.
struct DeletionRecordFlag {
    // TYPES
    enum Enum {
        e_NONE             = 0,
        e_IMPLICIT_CONFIRM = 1,
        e_TTL_EXPIRATION   = 2,
        e_NO_SC_QUORUM     = 4
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// DeletionRecordFlag::Enum' value.
    static bsl::ostream& print(bsl::ostream&            stream,
                               DeletionRecordFlag::Enum value,
                               int                      level          = 0,
                               int                      spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << DeletionRecordFlag::toAscii(
    ///                              DeletionRecordFlag::e_TTL_EXPIRATION)
    /// ```
    /// will print the following on standard output:
    /// ```
    /// TTL_EXPIRATION
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(DeletionRecordFlag::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, DeletionRecordFlag::Enum value);

// =====================
// struct DeletionRecord
// =====================

/// This struct represents a DeletionRecord present in the `JOURNAL` file of
/// a BlazingMQ file store.
struct DeletionRecord {
    // DeletionRecord structure datagram [60 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                       Reserved                |   QueueKey    |
    //   +---------------+---------------+---------------+---------------+
    //   |                            QueueKey                           |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                              GUID                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Magic                             |
    //   +---------------+---------------+---------------+---------------+
    //
    //  Header................: Record header
    //  QueueKey..............: Queue key to which this message record belongs
    //  GUID..................: Message GUID
    //  Magic.................: Magic word
    //..
    //
    // Note that DeletionRecordFlag is stored in the 'flags' field of the
    // 'RecordHeader'.

  private:
    // DATA
    RecordHeader d_header;

    char d_reserved1[3];

    char d_queueKey[FileStoreProtocol::k_KEY_LENGTH];

    unsigned char d_guid[bmqt::MessageGUID::e_SIZE_BINARY];

    char d_reserved2[12];

    bdlb::BigEndianUint32 d_magic;

  public:
    // CREATORS

    /// Create an instance with all fields unset.
    DeletionRecord();

    // MANIPULATORS
    RecordHeader& header();

    DeletionRecord& setDeletionRecordFlag(DeletionRecordFlag::Enum value);

    DeletionRecord& setQueueKey(const mqbu::StorageKey& key);

    DeletionRecord& setMessageGUID(const bmqt::MessageGUID& value);

    DeletionRecord& setMagic(unsigned int value);

    // ACCESSORS
    const RecordHeader& header() const;

    DeletionRecordFlag::Enum deletionRecordFlag() const;

    const mqbu::StorageKey& queueKey() const;

    const bmqt::MessageGUID& messageGUID() const;

    unsigned int magic() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const DeletionRecord& rhs);

// ==================
// struct QueueOpType
// ==================

/// This struct defines the type of queue operations in QueueOpRecord.
struct QueueOpType {
    // TYPES
    enum Enum {
        e_UNDEFINED = 0,
        e_PURGE     = 1  // A queue (or a specific appId) is purged
        ,
        e_CREATION = 2  // A new queue is created
        ,
        e_DELETION = 3  // A queue (or a specific appId) is deleted
        ,
        e_ADDITION = 4  // New appId(s) have been added to existing queue
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a `QueueOpType::Enum`
    /// value.
    static bsl::ostream& print(bsl::ostream&     stream,
                               QueueOpType::Enum value,
                               int               level          = 0,
                               int               spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << QueueOpType::toAscii(QueueOpType::e_PURGE)
    /// ```
    /// will print the following on standard output:
    /// ```
    /// PURGE
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(QueueOpType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, QueueOpType::Enum value);

// ====================
// struct QueueOpRecord
// ====================

/// This struct represents a QueueOpRecord present in the `JOURNAL` file of
/// a BlazingMQ file store.
struct QueueOpRecord {
    // QueueOpRecord structure datagram [60 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |           Reserved            |           QueueKey            |
    //   +---------------+---------------+---------------+---------------+
    //   |                   QueueKey                    |    AppKey     |
    //   +---------------+---------------+---------------+---------------+
    //   |                            AppKey                             |
    //   +---------------+---------------+---------------+---------------+
    //   |                           QueueOpType                         |
    //   +---------------+---------------+---------------+---------------+
    //   |                   QueueUriRecordOffsetWords                   |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Magic                             |
    //   +---------------+---------------+---------------+---------------+
    //
    //  Header.....................: Record header
    //  QueueKey...................: Queue key to which this message record
    //                               belongs
    //  AppKey.....................: App key to which this message record
    //                               belongs.  This field can be null.
    //  QueueOpType................: QueueOpType
    //  QueueUriRecordOffsetWords..: Offset (in WORDS) of the QueueUriRecord in
    //                               the QLIST file.  Valid only if
    //                               QueueOpType == CREATION or ADDITION.
    //  Magic......................: Magic word
    //..
    //
    // Note that if 'AppKey' is specified, this queue operation event is
    // applicable only to that 'AppKey'.  Also note that if 'QueueOpType' is of
    // type 'CREATION' or 'ADDITION', then a corresponding record is also
    // present in the QLIST file at offset 'QueueUriRecordOffsetWords'.

  private:
    // DATA
    RecordHeader d_header;

    char d_reserved1[2];

    char d_queueKey[FileStoreProtocol::k_KEY_LENGTH];

    char d_appKey[FileStoreProtocol::k_KEY_LENGTH];

    bdlb::BigEndianInt32 d_queueOpType;

    bdlb::BigEndianUint32 d_queueUriRecordOffsetWords;

    char d_reserved2[16];

    bdlb::BigEndianUint32 d_magic;

  public:
    // CREATORS

    /// Create an instance with all fields unset.
    QueueOpRecord();

    // MANIPULATORS
    RecordHeader& header();

    QueueOpRecord& setFlags(unsigned int value);

    QueueOpRecord& setQueueKey(const mqbu::StorageKey& key);

    QueueOpRecord& setAppKey(const mqbu::StorageKey& key);

    QueueOpRecord& setType(QueueOpType::Enum value);

    QueueOpRecord& setQueueUriRecordOffsetWords(unsigned int value);

    QueueOpRecord& setMagic(unsigned int value);

    // ACCESSORS
    const RecordHeader& header() const;

    unsigned int flags() const;

    const mqbu::StorageKey& queueKey() const;

    const mqbu::StorageKey& appKey() const;

    QueueOpType::Enum type() const;

    unsigned int queueUriRecordOffsetWords() const;

    unsigned int magic() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const QueueOpRecord& rhs);

// ====================
// struct JournalOpType
// ====================

/// This struct defines the type of journal operations in JournalOpRecord.
struct JournalOpType {
    // TYPES
    enum Enum {
        e_UNDEFINED = 0,
        e_UNUSED    = 1  // Can be used in future.
        ,
        e_SYNCPOINT = 2
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `JournalOpType::Enum` value.
    static bsl::ostream& print(bsl::ostream&       stream,
                               JournalOpType::Enum value,
                               int                 level          = 0,
                               int                 spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << JournalOpType::toAscii(JournalOpType::e_SYNCPOINT)
    /// ```
    /// will print the following on standard output:
    /// ```
    /// SYNCPOINT
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(JournalOpType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, JournalOpType::Enum value);

// ====================
// struct SyncPointType
// ====================

/// This struct defines the types of journal sync point records.
struct SyncPointType {
    // TYPES
    enum Enum { e_UNDEFINED = 0, e_REGULAR = 1, e_ROLLOVER = 2 };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `SyncPointType::Enum` value.
    static bsl::ostream& print(bsl::ostream&       stream,
                               SyncPointType::Enum value,
                               int                 level          = 0,
                               int                 spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << SyncPointType::toAscii(SyncPointType::e_REGULAR)
    /// ```
    /// will print the following on standard output:
    /// ```
    /// REGULAR
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(SyncPointType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, SyncPointType::Enum value);

// ======================
// struct JournalOpRecord
// ======================

/// This struct represents a JournalOpRecord present in the `JOURNAL` file
/// of a BlazingMQ file store.
struct JournalOpRecord {
    // JournalOpRecord structure datagram [60 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Header                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                    Reserved                   | SyncPointType |
    //   +---------------+---------------+---------------+---------------+
    //   |                       JournalOpType                           |
    //   +---------------+---------------+---------------+---------------+
    //   |                   Sequence Number Upper Bits                  |
    //   +---------------+---------------+---------------+---------------+
    //   |                   Sequence Number Lower Bits                  |
    //   +---------------+---------------+---------------+---------------+
    //   |                       Primary Node Id                         |
    //   +---------------+---------------+---------------+---------------+
    //   |                       Primary Lease Id                        |
    //   +---------------+---------------+---------------+---------------+
    //   |                     DataFileOffsetDwords                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                     QlistFileOffsetWords                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Reserved                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                             Magic                             |
    //   +---------------+---------------+---------------+---------------+
    //
    //  Header......................: Record header
    //  SyncPointType...............: Type of sync point record.  Valid only
    //                                for JournalOpType::e_SYNCPOINT
    //  JournalOpType...............: JournalOpType
    //  Sequence Number Upper Bits..: Upper 32 bits of sequence number. Valid
    //                                only for JournalOpType::e_SYNCPOINT
    //  Sequence Number Lower Bits..: Lower 32 bits of sequence number. Valid
    //                                only for JournalOpType::e_SYNCPOINT
    //  Primary Node Id.............: NodeId of partition's primary. Valid only
    //                                for JournalOpType::e_SYNCPOINT
    //  Primary Lease Id............: LeaseId of the primary of this partition.
    //                                Valid only for JournalOpType::e_SYNCPOINT
    //  DataFileOffstDwords.........: Current offset (in DWORDs) of the data
    //                                file.  Valid only for
    //                                JournalOpType::e_SYNCPOINT.
    //  QlistFileOffsetWords........: Current offset (in WORDs) of the qlist
    //                                file.  Valid only for
    //                                JournalOpType::e_SYNCPOINT.
    //  Magic.......................: Magic word
    //..

  private:
    // DATA
    RecordHeader d_header;

    char d_reserved1[3];

    unsigned char d_syncPointType;

    bdlb::BigEndianInt32 d_journalOpType;

    bdlb::BigEndianUint32 d_seqNumUpperBits;

    bdlb::BigEndianUint32 d_seqNumLowerBits;

    bdlb::BigEndianInt32 d_primaryNodeId;

    bdlb::BigEndianUint32 d_primaryLeaseId;

    bdlb::BigEndianUint32 d_dataFileOffsetDwords;

    bdlb::BigEndianUint32 d_qlistFileOffsetWords;

    char d_reserved2[4];

    bdlb::BigEndianUint32 d_magic;

  public:
    // CREATORS

    /// Create an instance with all fields unset.
    JournalOpRecord();

    /// Create an instance and initialize corresponding fields respectively
    /// with the specified `type` and `magic` values.  All other fields are
    /// set to zero.
    JournalOpRecord(JournalOpType::Enum type, unsigned int magic);

    /// Create an instance and initialize corresponding fields respectively
    /// with the specified `type`, `sequenceNum`, `primaryNodeId`,
    /// `primaryLeaseId` and `magic` values.
    JournalOpRecord(JournalOpType::Enum type,
                    SyncPointType::Enum syncPointType,
                    bsls::Types::Uint64 sequenceNum,
                    int                 primaryNodeId,
                    unsigned int        primaryLeaseId,
                    unsigned int        dataFileOffsetDwords,
                    unsigned int        qlistFileOffsetWords,
                    unsigned int        magic);

    // MANIPULATORS
    RecordHeader& header();

    JournalOpRecord& setFlags(unsigned int value);

    JournalOpRecord& setType(JournalOpType::Enum value);

    JournalOpRecord& setSyncPointType(SyncPointType::Enum value);

    JournalOpRecord& setSequenceNum(bsls::Types::Uint64 value);

    JournalOpRecord& setPrimaryNodeId(int value);

    JournalOpRecord& setPrimaryLeaseId(unsigned int value);

    JournalOpRecord& setDataFileOffsetDwords(unsigned int value);

    JournalOpRecord& setQlistFileOffsetWords(unsigned int value);

    JournalOpRecord& setMagic(unsigned int value);

    // ACCESSORS
    const RecordHeader& header() const;

    unsigned int flags() const;

    JournalOpType::Enum type() const;

    SyncPointType::Enum syncPointType() const;

    bsls::Types::Uint64 sequenceNum() const;

    int primaryNodeId() const;

    unsigned int primaryLeaseId() const;

    unsigned int dataFileOffsetDwords() const;

    unsigned int qlistFileOffsetWords() const;

    unsigned int magic() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const JournalOpRecord& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// struct FileHeader
// -----------------

// CREATORS
inline FileHeader::FileHeader()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(FileHeader));
    setProtocolVersion(FileStoreProtocol::k_VERSION);
    setHeaderWords(sizeof(FileHeader) / bmqp::Protocol::k_WORD_SIZE);
    setBitness(static_cast<Bitness::Enum>(sizeof(size_t) / 4));
    setMagic1(FileHeader::k_MAGIC1);
    setMagic2(FileHeader::k_MAGIC2);

    (void)d_reserved1[0];  // warning: private field 'd_reserved1' is not used
    (void)d_reserved2[0];  // warning: private field 'd_reserved2' is not used
}

// MANIPULATORS
inline FileHeader& FileHeader::setMagic1(unsigned int value)
{
    d_magic1 = value;
    return *this;
}

inline FileHeader& FileHeader::setMagic2(unsigned int value)
{
    d_magic2 = value;
    return *this;
}

inline FileHeader& FileHeader::setProtocolVersion(unsigned char value)
{
    d_protoVerAndHeaderWords = static_cast<unsigned char>(
        (d_protoVerAndHeaderWords & k_HEADER_WORDS_MASK) |
        (value << k_PROTOCOL_VERSION_START_IDX));
    return *this;
}

inline FileHeader& FileHeader::setHeaderWords(unsigned char value)
{
    d_protoVerAndHeaderWords = static_cast<unsigned char>(
        (d_protoVerAndHeaderWords & k_PROTOCOL_VERSION_MASK) |
        (value & k_HEADER_WORDS_MASK));
    return *this;
}

inline FileHeader& FileHeader::setBitness(Bitness::Enum value)
{
    if (Bitness::e_64 == value) {
        d_bitnessAndFileType = static_cast<char>(
            (d_bitnessAndFileType & k_FILE_TYPE_MASK) |
            (1 << k_BITNESS_START_IDX));
    }
    else {
        d_bitnessAndFileType = static_cast<char>(d_bitnessAndFileType &
                                                 k_FILE_TYPE_MASK);
    }

    return *this;
}

inline FileHeader& FileHeader::setFileType(FileType::Enum value)
{
    d_bitnessAndFileType = static_cast<char>(
        (d_bitnessAndFileType & k_BITNESS_MASK) | (value & k_FILE_TYPE_MASK));

    return *this;
}

inline FileHeader& FileHeader::setPartitionId(int value)
{
    d_partitionId = value;
    return *this;
}

// ACCESSORS
inline unsigned int FileHeader::magic1() const
{
    return d_magic1;
}

inline unsigned int FileHeader::magic2() const
{
    return d_magic2;
}

inline unsigned char FileHeader::protocolVersion() const
{
    return static_cast<unsigned char>(
        (d_protoVerAndHeaderWords & k_PROTOCOL_VERSION_MASK) >>
        k_PROTOCOL_VERSION_START_IDX);
}

inline unsigned char FileHeader::headerWords() const
{
    return static_cast<unsigned char>(d_protoVerAndHeaderWords &
                                      k_HEADER_WORDS_MASK);
}

inline Bitness::Enum FileHeader::bitness() const
{
    if (0 ==
        ((d_bitnessAndFileType & k_BITNESS_MASK) >> k_BITNESS_START_IDX)) {
        return Bitness::e_32;  // RETURN
    }

    return Bitness::e_64;
}

inline FileType::Enum FileHeader::fileType() const
{
    return static_cast<FileType::Enum>(d_bitnessAndFileType &
                                       k_FILE_TYPE_MASK);
}

inline int FileHeader::partitionId() const
{
    return d_partitionId;
}

// ---------------------
// struct DataFileHeader
// ---------------------

// CREATORS
inline DataFileHeader::DataFileHeader()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(DataFileHeader));
    setHeaderWords(sizeof(DataFileHeader) / bmqp::Protocol::k_WORD_SIZE);
    setFileKey(mqbu::StorageKey::k_NULL_KEY);

    // Suppress "unused variable" warning
    (void)d_reserved1;
    (void)d_reserved2;
}

// MANIPULATORS
inline DataFileHeader& DataFileHeader::setHeaderWords(unsigned char value)
{
    d_headerWords = value;
    return *this;
}

inline DataFileHeader&
DataFileHeader::setFileKey(const mqbu::StorageKey& value)
{
    bsl::memcpy(d_fileKey, value.data(), FileStoreProtocol::k_KEY_LENGTH);
    return *this;
}

// ACCESSORS
inline unsigned char DataFileHeader::headerWords() const
{
    return d_headerWords;
}

inline const mqbu::StorageKey& DataFileHeader::fileKey() const
{
    return reinterpret_cast<const mqbu::StorageKey&>(d_fileKey);
}

// ------------------------
// struct JournalFileHeader
// ------------------------

// CREATORS
inline JournalFileHeader::JournalFileHeader()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(JournalFileHeader));
    setHeaderWords(sizeof(JournalFileHeader) / bmqp::Protocol::k_WORD_SIZE);
    setRecordWords(FileStoreProtocol::k_JOURNAL_RECORD_SIZE /
                   bmqp::Protocol::k_WORD_SIZE);
    setFirstSyncPointOffsetWords(0);
    (void)d_reserved;  // suppress 'unused variable' compiler diagnostic
}

// MANIPULATORS
inline JournalFileHeader&
JournalFileHeader::setHeaderWords(unsigned char value)
{
    d_headerWords = value;
    return *this;
}

inline JournalFileHeader&
JournalFileHeader::setRecordWords(unsigned char value)
{
    d_recordWords = value;
    return *this;
}

inline JournalFileHeader&
JournalFileHeader::setFirstSyncPointOffsetWords(bsls::Types::Uint64 value)
{
    bmqp::Protocol::split(&d_firstSyncPointOffsetUpperBits,
                          &d_firstSyncPointOffsetLowerBits,
                          value);
    return *this;
}

// ACCESSORS
inline unsigned char JournalFileHeader::headerWords() const
{
    return d_headerWords;
}

inline unsigned char JournalFileHeader::recordWords() const
{
    return d_recordWords;
}

inline bsls::Types::Uint64 JournalFileHeader::firstSyncPointOffsetWords() const
{
    return bmqp::Protocol::combine(d_firstSyncPointOffsetUpperBits,
                                   d_firstSyncPointOffsetLowerBits);
}

// ----------------------
// struct QlistFileHeader
// ----------------------

// CREATORS
inline QlistFileHeader::QlistFileHeader()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(QlistFileHeader));
    setHeaderWords(sizeof(QlistFileHeader) / bmqp::Protocol::k_WORD_SIZE);
    (void)d_reserved[0];  // warning: private field 'd_reserved' is not used
}

// MANIPULATORS
inline QlistFileHeader& QlistFileHeader::setHeaderWords(unsigned char value)
{
    d_headerWords = value;

    return *this;
}

// ACCESSORS
inline unsigned char QlistFileHeader::headerWords() const
{
    return d_headerWords;
}

// -------------------------
// struct DataHeaderFlagUtil
// -------------------------

inline void DataHeaderFlagUtil::setFlag(int* flags, DataHeaderFlags::Enum flag)
{
    *flags = (*flags | flag);
}

inline void DataHeaderFlagUtil::unsetFlag(int*                  flags,
                                          DataHeaderFlags::Enum flag)
{
    *flags &= ~flag;
}

inline bool DataHeaderFlagUtil::isSet(int flags, DataHeaderFlags::Enum flag)
{
    return (0 != (flags & flag));
}

// -----------------
// struct DataHeader
// -----------------

// CREATORS
inline DataHeader::DataHeader()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(DataHeader));
    const size_t size = sizeof(DataHeader) / bmqp::Protocol::k_WORD_SIZE;
    setHeaderWords(size);
    setMessageWords(size);

    (void)d_reserved;
}

// MANIPULATORS
inline DataHeader& DataHeader::setHeaderWords(int value)
{
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_HEADER_WORDS_NUM_BITS) - 1);

    d_headerWordsAndMessageWords = (d_headerWordsAndMessageWords &
                                    k_MSG_WORDS_MASK) |
                                   (value << k_HEADER_WORDS_START_IDX);
    return *this;
}

inline DataHeader& DataHeader::setMessageWords(int value)
{
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_MSG_WORDS_NUM_BITS) - 1);

    d_headerWordsAndMessageWords = (d_headerWordsAndMessageWords &
                                    k_HEADER_WORDS_MASK) |
                                   (value & k_MSG_WORDS_MASK);
    return *this;
}

inline DataHeader& DataHeader::setOptionsWords(int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_OPTIONS_WORDS_NUM_BITS) - 1);

    d_optionsWordsAndFlags = (d_optionsWordsAndFlags & k_FLAGS_MASK) |
                             (value << k_OPTIONS_WORDS_START_IDX);

    return *this;
}

inline DataHeader& DataHeader::setFlags(int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_FLAGS_NUM_BITS) - 1);

    d_optionsWordsAndFlags = (d_optionsWordsAndFlags & k_OPTIONS_WORDS_MASK) |
                             (value & k_FLAGS_MASK);
    return *this;
}

inline bmqp::SchemaWireId& DataHeader::schemaId()
{
    return d_schemaId;
}

// ACCESSORS
inline int DataHeader::headerWords() const
{
    return (d_headerWordsAndMessageWords & k_HEADER_WORDS_MASK) >>
           k_HEADER_WORDS_START_IDX;
}

inline int DataHeader::messageWords() const
{
    return d_headerWordsAndMessageWords & k_MSG_WORDS_MASK;
}

inline int DataHeader::optionsWords() const
{
    return (d_optionsWordsAndFlags & k_OPTIONS_WORDS_MASK) >>
           k_OPTIONS_WORDS_START_IDX;
}

inline int DataHeader::flags() const
{
    return d_optionsWordsAndFlags & k_FLAGS_MASK;
}

inline const bmqp::SchemaWireId& DataHeader::schemaId() const
{
    return d_schemaId;
}

// ------------------
// struct AppIdHeader
// ------------------

// CREATORS
inline AppIdHeader::AppIdHeader()
: d_reserved(bdlb::BigEndianUint16::make(0))
, d_appIdLengthWords(bdlb::BigEndianUint16::make(0))
{
}

// MANIPULATORS
inline AppIdHeader& AppIdHeader::setAppIdLengthWords(unsigned int value)
{
    d_appIdLengthWords = static_cast<unsigned short>(value);
    return *this;
}

// ACCESSORS
inline unsigned int AppIdHeader::appIdLengthWords() const
{
    return d_appIdLengthWords;
}

// ------------------------
// struct QueueRecordHeader
// ------------------------

// CREATORS
inline QueueRecordHeader::QueueRecordHeader()
: d_queueUriLengthWords(bdlb::BigEndianUint16::make(0))
, d_numAppIds(bdlb::BigEndianUint16::make(0))
, d_headerWordsAndQueueRecordWords(bdlb::BigEndianUint32::make(0))
{
    const size_t val = sizeof(QueueRecordHeader) / bmqp::Protocol::k_WORD_SIZE;
    setHeaderWords(val);
    setQueueRecordWords(val);
    bsl::memset(d_reserved, 0, k_RESERVED_FIELD_NUM_BYTES);
}

// MANIPULATORS
inline QueueRecordHeader&
QueueRecordHeader::setQueueUriLengthWords(unsigned int value)
{
    d_queueUriLengthWords = static_cast<unsigned short>(value);
    return *this;
}

inline QueueRecordHeader& QueueRecordHeader::setNumAppIds(unsigned int value)
{
    d_numAppIds = static_cast<unsigned short>(value);
    return *this;
}

inline QueueRecordHeader& QueueRecordHeader::setHeaderWords(unsigned int value)
{
    d_headerWordsAndQueueRecordWords = (value << k_HEADER_WORDS_START_IDX) |
                                       (d_headerWordsAndQueueRecordWords &
                                        k_QUEUE_RECORD_WORDS_MASK);

    return *this;
}

inline QueueRecordHeader&
QueueRecordHeader::setQueueRecordWords(unsigned int value)
{
    d_headerWordsAndQueueRecordWords = (d_headerWordsAndQueueRecordWords &
                                        k_HEADER_WORDS_MASK) |
                                       (value & k_QUEUE_RECORD_WORDS_MASK);

    return *this;
}

// ACCESSORS
inline unsigned int QueueRecordHeader::queueUriLengthWords() const
{
    return d_queueUriLengthWords;
}

inline unsigned int QueueRecordHeader::numAppIds() const
{
    return d_numAppIds;
}

inline unsigned int QueueRecordHeader::headerWords() const
{
    return (d_headerWordsAndQueueRecordWords & k_HEADER_WORDS_MASK) >>
           k_HEADER_WORDS_START_IDX;
}

inline unsigned int QueueRecordHeader::queueRecordWords() const
{
    return d_headerWordsAndQueueRecordWords & k_QUEUE_RECORD_WORDS_MASK;
}

// -------------------
// struct RecordHeader
// -------------------

// CREATORS
inline RecordHeader::RecordHeader()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(RecordHeader));
}

// MANIPULATORS
inline RecordHeader& RecordHeader::setType(RecordType::Enum value)
{
    d_typeAndFlags = static_cast<unsigned short>(
        (d_typeAndFlags & k_FLAGS_MASK) | (value << k_TYPE_START_IDX));
    return *this;
}

inline RecordHeader& RecordHeader::setFlags(unsigned int value)
{
    d_typeAndFlags = static_cast<unsigned short>(
        (d_typeAndFlags & k_TYPE_MASK) | value);
    return *this;
}

inline RecordHeader& RecordHeader::setPrimaryLeaseId(unsigned int value)
{
    d_primaryLeaseId = value;
    return *this;
}

inline RecordHeader& RecordHeader::setSequenceNumber(bsls::Types::Uint64 value)
{
    bmqp::Protocol::split(&d_seqNumUpperBits, &d_seqNumLowerBits, value);
    return *this;
}

inline RecordHeader& RecordHeader::setTimestamp(bsls::Types::Uint64 value)
{
    bmqp::Protocol::split(&d_timestampUpperBits, &d_timestampLowerBits, value);
    return *this;
}

// ACCESSORS
inline RecordType::Enum RecordHeader::type() const
{
    return static_cast<RecordType::Enum>((d_typeAndFlags & k_TYPE_MASK) >>
                                         k_TYPE_START_IDX);
}

inline unsigned int RecordHeader::flags() const
{
    return d_typeAndFlags & k_FLAGS_MASK;
}

inline unsigned int RecordHeader::primaryLeaseId() const
{
    return d_primaryLeaseId;
}

inline bsls::Types::Uint64 RecordHeader::sequenceNumber() const
{
    return bmqp::Protocol::combine(
        static_cast<unsigned int>(d_seqNumUpperBits),
        d_seqNumLowerBits);
}

inline bsls::Types::Uint64 RecordHeader::timestamp() const
{
    return bmqp::Protocol::combine(d_timestampUpperBits, d_timestampLowerBits);
}

inline bmqp_ctrlmsg::PartitionSequenceNumber
RecordHeader::partitionSequenceNumber() const
{
    bmqp_ctrlmsg::PartitionSequenceNumber partitionSequenceNumber;

    partitionSequenceNumber.primaryLeaseId() = primaryLeaseId();
    partitionSequenceNumber.sequenceNumber() = sequenceNumber();

    return partitionSequenceNumber;
}

// --------------------
// struct MessageRecord
// --------------------

// CREATORS
inline MessageRecord::MessageRecord()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(MessageRecord));
    d_header.setType(RecordType::e_MESSAGE);
    setQueueKey(mqbu::StorageKey::k_NULL_KEY);
    setFileKey(mqbu::StorageKey::k_NULL_KEY);
    static_cast<void>(d_reserved);
}

// MANIPULATORS
inline RecordHeader& MessageRecord::header()
{
    return d_header;
}

inline MessageRecord& MessageRecord::setRefCount(unsigned int value)
{
    d_header.setFlags(value);
    return *this;
}

inline MessageRecord& MessageRecord::setCompressionAlgorithmType(
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(
        compressionAlgorithmType >=
            bmqt::CompressionAlgorithmType::k_LOWEST_SUPPORTED_TYPE &&
        compressionAlgorithmType <=
            bmqt::CompressionAlgorithmType::k_HIGHEST_SUPPORTED_TYPE);

    d_reservedAndCAT = 0;
    d_reservedAndCAT = d_reservedAndCAT |
                       static_cast<char>(compressionAlgorithmType
                                         << k_CAT_START_IDX);
    return *this;
}

inline MessageRecord& MessageRecord::setQueueKey(const mqbu::StorageKey& key)
{
    bsl::memcpy(d_queueKey, key.data(), mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    return *this;
}

inline MessageRecord& MessageRecord::setFileKey(const mqbu::StorageKey& key)
{
    bsl::memcpy(d_fileKey, key.data(), mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    return *this;
}

inline MessageRecord& MessageRecord::setMessageOffsetDwords(unsigned int value)
{
    d_messageOffsetDwords = value;
    return *this;
}

inline MessageRecord&
MessageRecord::setMessageGUID(const bmqt::MessageGUID& value)
{
    value.toBinary(d_guid);
    return *this;
}

inline MessageRecord& MessageRecord::setCrc32c(unsigned int value)
{
    d_crc32c = value;
    return *this;
}

inline MessageRecord& MessageRecord::setMagic(unsigned int value)
{
    d_magic = value;
    return *this;
}

// ACCESSORS
inline const RecordHeader& MessageRecord::header() const
{
    return d_header;
}

inline unsigned int MessageRecord::refCount() const
{
    return d_header.flags();
}

inline bmqt::CompressionAlgorithmType::Enum
MessageRecord::compressionAlgorithmType() const
{
    int value = (k_CAT_MASK & d_reservedAndCAT) >> k_CAT_START_IDX;
    return static_cast<bmqt::CompressionAlgorithmType::Enum>(value);
}

inline const mqbu::StorageKey& MessageRecord::queueKey() const
{
    return reinterpret_cast<const mqbu::StorageKey&>(d_queueKey);
}

inline const mqbu::StorageKey& MessageRecord::fileKey() const
{
    return reinterpret_cast<const mqbu::StorageKey&>(d_fileKey);
}

inline unsigned int MessageRecord::messageOffsetDwords() const
{
    return d_messageOffsetDwords;
}

inline const bmqt::MessageGUID& MessageRecord::messageGUID() const
{
    return reinterpret_cast<const bmqt::MessageGUID&>(d_guid);
}

inline unsigned int MessageRecord::crc32c() const
{
    return d_crc32c;
}

inline unsigned int MessageRecord::magic() const
{
    return d_magic;
}

// --------------------
// struct ConfirmRecord
// --------------------

// CREATORS
inline ConfirmRecord::ConfirmRecord()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(ConfirmRecord));
    d_header.setType(RecordType::e_CONFIRM);
    setQueueKey(mqbu::StorageKey::k_NULL_KEY);
    setAppKey(mqbu::StorageKey::k_NULL_KEY);

    (void)d_reserved1[0];  // warning: private field 'd_reserved' is not used
    (void)d_reserved2[0];  // warning: private field 'd_reserved' is not used
}

// MANIPULATORS
inline RecordHeader& ConfirmRecord::header()
{
    return d_header;
}

inline ConfirmRecord& ConfirmRecord::setReason(ConfirmReason::Enum value)
{
    d_header.setFlags(static_cast<int>(value));
    return *this;
}

inline ConfirmRecord& ConfirmRecord::setQueueKey(const mqbu::StorageKey& key)
{
    bsl::memcpy(d_queueKey, key.data(), mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    return *this;
}

inline ConfirmRecord& ConfirmRecord::setAppKey(const mqbu::StorageKey& key)
{
    bsl::memcpy(d_appKey, key.data(), mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    return *this;
}

inline ConfirmRecord&
ConfirmRecord::setMessageGUID(const bmqt::MessageGUID& value)
{
    value.toBinary(d_guid);
    return *this;
}

inline ConfirmRecord& ConfirmRecord::setMagic(unsigned int value)
{
    d_magic = value;
    return *this;
}

// ACCESSORS
inline const RecordHeader& ConfirmRecord::header() const
{
    return d_header;
}

inline ConfirmReason::Enum ConfirmRecord::reason() const
{
    return static_cast<ConfirmReason::Enum>(d_header.flags());
}

inline const mqbu::StorageKey& ConfirmRecord::queueKey() const
{
    return reinterpret_cast<const mqbu::StorageKey&>(d_queueKey);
}

inline const mqbu::StorageKey& ConfirmRecord::appKey() const
{
    return reinterpret_cast<const mqbu::StorageKey&>(d_appKey);
}

inline const bmqt::MessageGUID& ConfirmRecord::messageGUID() const
{
    return reinterpret_cast<const bmqt::MessageGUID&>(d_guid);
}

inline unsigned int ConfirmRecord::magic() const
{
    return d_magic;
}

// ---------------------
// struct DeletionRecord
// ---------------------

// CREATORS
inline DeletionRecord::DeletionRecord()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(DeletionRecord));
    d_header.setType(RecordType::e_DELETION);
    setQueueKey(mqbu::StorageKey::k_NULL_KEY);

    (void)d_reserved1[0];  // warning: private field 'd_reserved1' is not used
    (void)d_reserved2[0];  // warning: private field 'd_reserved2' is not used
}

// MANIPULATORS
inline RecordHeader& DeletionRecord::header()
{
    return d_header;
}

inline DeletionRecord&
DeletionRecord::setDeletionRecordFlag(DeletionRecordFlag::Enum value)
{
    d_header.setFlags(static_cast<int>(value));
    return *this;
}

inline DeletionRecord& DeletionRecord::setQueueKey(const mqbu::StorageKey& key)
{
    bsl::memcpy(d_queueKey, key.data(), mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    return *this;
}

inline DeletionRecord&
DeletionRecord::setMessageGUID(const bmqt::MessageGUID& value)
{
    value.toBinary(d_guid);
    return *this;
}

inline DeletionRecord& DeletionRecord::setMagic(unsigned int value)
{
    d_magic = value;
    return *this;
}

// ACCESSORS
inline const RecordHeader& DeletionRecord::header() const
{
    return d_header;
}

inline DeletionRecordFlag::Enum DeletionRecord::deletionRecordFlag() const
{
    return static_cast<DeletionRecordFlag::Enum>(d_header.flags());
}

inline const mqbu::StorageKey& DeletionRecord::queueKey() const
{
    return reinterpret_cast<const mqbu::StorageKey&>(d_queueKey);
}

inline const bmqt::MessageGUID& DeletionRecord::messageGUID() const
{
    return reinterpret_cast<const bmqt::MessageGUID&>(d_guid);
}

inline unsigned int DeletionRecord::magic() const
{
    return d_magic;
}

// --------------------
// struct QueueOpRecord
// --------------------

// CREATORS
inline QueueOpRecord::QueueOpRecord()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(QueueOpRecord));
    d_header.setType(RecordType::e_QUEUE_OP);
    setQueueKey(mqbu::StorageKey::k_NULL_KEY);
    setAppKey(mqbu::StorageKey::k_NULL_KEY);

    (void)d_reserved1[0];  // warning: private field 'd_reserved' is not used
    (void)d_reserved2[0];  // warning: private field 'd_reserved' is not used
}

// MANIPULATORS
inline RecordHeader& QueueOpRecord::header()
{
    return d_header;
}

inline QueueOpRecord& QueueOpRecord::setFlags(unsigned int value)
{
    d_header.setFlags(value);
    return *this;
}

inline QueueOpRecord& QueueOpRecord::setQueueKey(const mqbu::StorageKey& key)
{
    bsl::memcpy(d_queueKey, key.data(), mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    return *this;
}

inline QueueOpRecord& QueueOpRecord::setAppKey(const mqbu::StorageKey& key)
{
    bsl::memcpy(d_appKey, key.data(), mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    return *this;
}

inline QueueOpRecord& QueueOpRecord::setType(QueueOpType::Enum value)
{
    d_queueOpType = static_cast<int>(value);
    return *this;
}

inline QueueOpRecord&
QueueOpRecord::setQueueUriRecordOffsetWords(unsigned int value)
{
    d_queueUriRecordOffsetWords = value;
    return *this;
}

inline QueueOpRecord& QueueOpRecord::setMagic(unsigned int value)
{
    d_magic = value;
    return *this;
}

// ACCESSORS
inline const RecordHeader& QueueOpRecord::header() const
{
    return d_header;
}

inline unsigned int QueueOpRecord::flags() const
{
    return d_header.flags();
}

inline const mqbu::StorageKey& QueueOpRecord::queueKey() const
{
    return reinterpret_cast<const mqbu::StorageKey&>(d_queueKey);
}

inline const mqbu::StorageKey& QueueOpRecord::appKey() const
{
    return reinterpret_cast<const mqbu::StorageKey&>(d_appKey);
}

inline QueueOpType::Enum QueueOpRecord::type() const
{
    return static_cast<QueueOpType::Enum>(static_cast<int>(d_queueOpType));
}

inline unsigned int QueueOpRecord::queueUriRecordOffsetWords() const
{
    return d_queueUriRecordOffsetWords;
}

inline unsigned int QueueOpRecord::magic() const
{
    return d_magic;
}

// ----------------------
// struct JournalOpRecord
// ----------------------

// CREATORS
inline JournalOpRecord::JournalOpRecord()
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(JournalOpRecord));
    d_header.setType(RecordType::e_JOURNAL_OP);

    (void)d_reserved1[0];  // warning: private field 'd_reserved1' is not used
    (void)d_reserved2[0];  // warning: private field 'd_reserved2' is not used
}

inline JournalOpRecord::JournalOpRecord(JournalOpType::Enum type,
                                        unsigned int        magic)
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(JournalOpRecord));
    d_header.setType(RecordType::e_JOURNAL_OP);
    setType(type);
    setMagic(magic);
}

inline JournalOpRecord::JournalOpRecord(JournalOpType::Enum type,
                                        SyncPointType::Enum syncPointType,
                                        bsls::Types::Uint64 sequenceNum,
                                        int                 primaryNodeId,
                                        unsigned int        primaryLeaseId,
                                        unsigned int dataFileOffsetDwords,
                                        unsigned int qlistFileOffsetWords,
                                        unsigned int magic)
{
    bsl::memset(reinterpret_cast<char*>(this), 0, sizeof(JournalOpRecord));
    d_header.setType(RecordType::e_JOURNAL_OP);
    setType(type);
    setSyncPointType(syncPointType);
    setSequenceNum(sequenceNum);
    setPrimaryNodeId(primaryNodeId);
    setPrimaryLeaseId(primaryLeaseId);
    setDataFileOffsetDwords(dataFileOffsetDwords);
    setQlistFileOffsetWords(qlistFileOffsetWords);
    setMagic(magic);
}

// MANIPULATORS
inline RecordHeader& JournalOpRecord::header()
{
    return d_header;
}

inline JournalOpRecord& JournalOpRecord::setFlags(unsigned int value)
{
    d_header.setFlags(value);
    return *this;
}

inline JournalOpRecord& JournalOpRecord::setType(JournalOpType::Enum value)
{
    d_journalOpType = static_cast<int>(value);
    return *this;
}

inline JournalOpRecord&
JournalOpRecord::setSyncPointType(SyncPointType::Enum value)
{
    d_syncPointType = static_cast<unsigned char>(value);
    return *this;
}

inline JournalOpRecord&
JournalOpRecord::setSequenceNum(bsls::Types::Uint64 value)
{
    bmqp::Protocol::split(&d_seqNumUpperBits, &d_seqNumLowerBits, value);
    return *this;
}

inline JournalOpRecord& JournalOpRecord::setPrimaryNodeId(int value)
{
    d_primaryNodeId = value;
    return *this;
}

inline JournalOpRecord& JournalOpRecord::setPrimaryLeaseId(unsigned int value)
{
    d_primaryLeaseId = value;
    return *this;
}

inline JournalOpRecord&
JournalOpRecord::setDataFileOffsetDwords(unsigned int value)
{
    d_dataFileOffsetDwords = value;
    return *this;
}

inline JournalOpRecord&
JournalOpRecord::setQlistFileOffsetWords(unsigned int value)
{
    d_qlistFileOffsetWords = value;
    return *this;
}

inline JournalOpRecord& JournalOpRecord::setMagic(unsigned int value)
{
    d_magic = value;
    return *this;
}

// ACCESSORS
inline const RecordHeader& JournalOpRecord::header() const
{
    return d_header;
}

inline unsigned int JournalOpRecord::flags() const
{
    return d_header.flags();
}

inline JournalOpType::Enum JournalOpRecord::type() const
{
    return static_cast<JournalOpType::Enum>(static_cast<int>(d_journalOpType));
}

inline SyncPointType::Enum JournalOpRecord::syncPointType() const
{
    return static_cast<SyncPointType::Enum>(d_syncPointType);
}

inline bsls::Types::Uint64 JournalOpRecord::sequenceNum() const
{
    return bmqp::Protocol::combine(d_seqNumUpperBits, d_seqNumLowerBits);
}

inline int JournalOpRecord::primaryNodeId() const
{
    return d_primaryNodeId;
}

inline unsigned int JournalOpRecord::primaryLeaseId() const
{
    return d_primaryLeaseId;
}

inline unsigned int JournalOpRecord::dataFileOffsetDwords() const
{
    return d_dataFileOffsetDwords;
}

inline unsigned int JournalOpRecord::qlistFileOffsetWords() const
{
    return d_qlistFileOffsetWords;
}

inline unsigned int JournalOpRecord::magic() const
{
    return d_magic;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mqbs::operator<<(bsl::ostream&       stream,
                                      mqbs::Bitness::Enum value)
{
    return mqbs::Bitness::print(stream, value, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&        stream,
                                      mqbs::FileType::Enum value)
{
    return mqbs::FileType::print(stream, value, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&               stream,
                                      mqbs::DataHeaderFlags::Enum value)
{
    return mqbs::DataHeaderFlags::print(stream, value, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&          stream,
                                      mqbs::RecordType::Enum value)
{
    return mqbs::RecordType::print(stream, value, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&       stream,
                                      const RecordHeader& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&        stream,
                                      const MessageRecord& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&             stream,
                                      mqbs::ConfirmReason::Enum value)
{
    return mqbs::ConfirmReason::print(stream, value, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&        stream,
                                      const ConfirmRecord& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&                  stream,
                                      mqbs::DeletionRecordFlag::Enum value)
{
    return mqbs::DeletionRecordFlag::print(stream, value, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&         stream,
                                      const DeletionRecord& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&           stream,
                                      mqbs::QueueOpType::Enum value)
{
    return mqbs::QueueOpType::print(stream, value, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&        stream,
                                      const QueueOpRecord& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&             stream,
                                      mqbs::JournalOpType::Enum value)
{
    return mqbs::JournalOpType::print(stream, value, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&             stream,
                                      mqbs::SyncPointType::Enum value)
{
    return mqbs::SyncPointType::print(stream, value, 0, -1);
}

inline bsl::ostream& mqbs::operator<<(bsl::ostream&          stream,
                                      const JournalOpRecord& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
