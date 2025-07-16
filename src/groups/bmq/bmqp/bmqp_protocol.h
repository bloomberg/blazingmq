// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqp_protocol.h                                                    -*-C++-*-
#ifndef INCLUDED_BMQP_PROTOCOL
#define INCLUDED_BMQP_PROTOCOL

//@PURPOSE: Provide definitions for BlazingMQ protocol structures and
// constants.
//
//@CLASSES
//  bmqp::RdaInfo        : VST representing a counter and status flags for the
//                         number of redelivery attempts remaining.
//  bmqp::SubQueueInfo   : VST representing a sub-queue receiving a message.
//  bmqp::Protocol       : Namespace for protocol generic values and routines.
//  bmqp::EventType      : Enum for types of the packets sent.
//  bmqp::EncodingType   : Enum for types of encoding used for control message.
//  bmqp::EncodingFeature: Field name of the encoding features and the list of
//                         supported encoding features.
//  bmqp::OptionType     : Enum for types of options for PUT or PUSH messages.
//  bmqp::EventHeader    : Header for a BlazingMQ event packet sent on the wire
//  bmqp::EventHeaderUtil: Utility methods for 'bmqp::EventHeader'.
//  bmqp::OptionHeader   : Header for an option in an event packet.
//  bmqp::MessagePropertiesHeader
//                       : Header for message properties area in PUT/PUSH msg.
//  bmqp::MessagePropertyHeader
//                       : Header for a message property in a PUT/PUSH message.
//  bmqp::PutHeader      : Header for mesages in PUT event packet.
//  bmqp::PutHeaderFlags : Meanings of each bit in flags field of 'PutHeader'.
//  bmqp::PutHeaderFlagUtil
//                       : Utility methods for 'bmqp::PutHeaderFlags'.
//  bmqp::AckHeader      : Header for messages in ACK event packet.
//  bmqp::AckHeaderFlags : Meanings of each bit in flags field of 'AckHeader'.
//  bmqp::AckMessage     : Structure of an ack msg (AckHeader payload).
//  bmqp::PushHeader     : Header for messages in PUSH event packet.
//  bmqp::PushHeaderFlags: Meanings of each bit in flags field of 'PushHeader'.
//  bmqp::PushHeaderFlagUtil
//                       : Utility methods for 'bmqp::PushHeaderFlags'.
//  bmqp::ConfirmHeader  : Header for messages in CONFIRM event packet.
//  bmqp::ConfirmMessage : Structure of a confirm msg (ConfirmHeader payload).
//  bmqp::RejectHeader   : Header for messages in REJECT event packet.
//  bmqp::RejectMessage  : Structure of a reject msg (RejectHeader payload).
//  bmqp::StorageMessageType
//                       : Enum for type of cluster storage messages exchanged.
//  bmqp::StorageHeader  : Header for a 'STORAGE' message.
//  bmqp::StorageHeaderFlags
//                       : Meanings of bits in flags field of 'StorageHeader'.
//  bmqp::StorageHeaderFlagUtil
//                       : Utility methods for 'bmqp::StorageHeaderFlags'.
//  bmqp::RecoveryFileChunkType
//                       : Enum for types that a file chunk can belong to
//                         during recovery phase.
//  bmqp::RecoveryHeader : Header for a 'RECOVERY' message.
//
//@SEE ALSO: bmqp_ctrlmsg.xsd
//
//@DESCRIPTION: This component provide definitions for a set of structures
// ('bmqp::AckHeader', 'bmqp::AckMessage', 'bmqp::ConfirmHeader',
// 'bmqp::ConfirmMessage', 'bmqp::RejectHeader', 'bmqp::RejectMessage',
// 'bmqp::EventHeader', 'bmqp::OptionHeader', 'bmqp::PushHeader' and
// 'bmqp::PutHeader') as well as an enum ('bmqp::EventType') defining the
// binary layout of the protocol messages used by BlazingMQ to communicate
// between client and broker, as well as between brokers.
//
//
/// Terminology
///===========
//
//: o A !message! is an application-level unit of data containing control
//:   information and possibly application payload data.
//
//: o An !event! is an application-level unit of data containing one or
//:   multiple !messages!.  The purpose of !events! is to batch !messages! for
//:   efficiency.  There are two types of events: !protocol! !events!
//:   representing for example open queue requests or acknowledgements, and
//:   !data! !events! carrying published data associated with a queue.
//
//: o A !packet! (a.k.a. network message) is a network packet, e.g. a formatted
//:   unit of data carried by the network.  A network interface object such as
//:   a Client Session sends and receives !packets! to peer processes.
//
//: o An !event! is sent over the network using one or multiple !packets!.  If
//:   the !event! is too big to fit in a single !packet!, it is sent using
//:   multiple !fragments!, each of them carrying a segment of the whole
//:   !event! data.  Therefore a !packet! contains either an !event! or a
//:   !fragment!.
//
//
/// Protocol Limitations
///====================
//..
//     =======================================================================
//     + Packet:
//       Max packet size.....................: 16 MB
//     -----------------------------------------------------------------------
//     + Fragment Header:
//       Max fragment header size.............: 28 bytes
//       Unique publisher Id..................: 8 bytes
//     -----------------------------------------------------------------------
//     + Event Header:
//       Max event size......................: 2 GB
//       Max event header....................: 1 020 bytes
//       Max concurrent protocol version.....: 4
//       Total different event type..........: 64
//     -----------------------------------------------------------------------
//     + Message Header:
//       Max message payload size............: ~1 GB (-messageHeader
//                                                    -options
//                                                    -messageProperties)
//       Max message header size.............: 124 bytes
//       Max options area length.............: 64 MB
//       Max message properties area length..: 64 MB
//       Max number of properties............: 255
//       Max types of property data types....: 31
//       Max length of a property name.......: 4 KB
//       Max length of a property value......: 64 MB
//       Unique message id...................: 16 bytes (GUID)
//     -----------------------------------------------------------------------
//     + Options:
//       Max option size.....................: 8 MB
//       Total different options type........: 64
//     -----------------------------------------------------------------------
//     + Tag List:
//       Max number of tags per tag list.....: 65 536
//       Total different tag id..............: 65 536
//       Max tag value length................: 64 MB
//       Max total TagList data length.......: ~64MB
//     ========================================================================
//..
//

// BMQ

#include <bmqc_array.h>
#include <bmqp_queueid.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_messageguid.h>

// BDE
#include <bdlb_bigendian.h>
#include <bsl_cstring.h>  // for bsl::memset & bsl::memcpy
#include <bsl_ostream.h>

#include <bsl_limits.h>
#include <bsl_string.h>  // for bslstl::StringRef
#include <bsla_annotations.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqp {

// ==============
// struct RdaInfo
// ==============

/// VST representing the RDA counter and flags, with the MSB being a flag
/// for unlimited retransmission and the second bit being a flag for a
/// potentially poisonous message.
struct RdaInfo {
    // FRIENDS
    friend bool operator==(const RdaInfo& lhs, const RdaInfo& rhs);

  private:
    // TYPES
    enum Enum { e_UNLIMITED = (1 << 7), e_POISONOUS = (1 << 6) };

  public:
    // CONSTANTS
    static const unsigned char k_MAX_COUNTER_VALUE;

    static const unsigned char k_MAX_INTERNAL_COUNTER_VALUE;

  private:
    // DATA
    unsigned char d_counter;

  public:
    // CREATORS

    /// Create an `RdaInfo` with unlimited set to true.
    RdaInfo();

    /// Create an `RdaInfo` with the counter and flags set to
    /// `internalRepresentation`. The behaviour is undefined when
    /// `internalRepresentation > 255`.
    RdaInfo(unsigned int internalRepresentation);

    /// Create an `RdaInfo` that has the same value as the specified
    /// `original` object.
    RdaInfo(const RdaInfo& original);

    // MANIPULATORS

    /// Set the unlimited delivery flag to true.
    RdaInfo& setUnlimited();

    /// Set the poisonous message flag to true.
    RdaInfo& setPotentiallyPoisonous(bool flag);

    /// Set the RDA counter to the specified `counter` value. The behaviour
    /// is undefined unless `0 <= counter <= 63`.
    RdaInfo& setCounter(unsigned int counter);

    // ACCESSORS

    /// Return whether the RDA counter is set to unlimited.
    bool isUnlimited() const;

    /// Return whether or not this message has been marked as potentially
    /// poisonous.
    bool isPotentiallyPoisonous() const;

    /// Return the value of the counter, or in the case the RDA counter is
    /// set to unlimited, a value higher than k_MAX_COUNTER_VALUE.
    unsigned int counter() const;

    /// Return the internal representation of the counter with its flags.
    unsigned int internalRepresentation() const;

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
bsl::ostream& operator<<(bsl::ostream& stream, const RdaInfo& rhs);

/// Return `true` if `rhs` object contains the value of the same type as
/// contained in `lhs` object and the value itself is the same in both
/// objects, return false otherwise.
bool operator==(const RdaInfo& lhs, const RdaInfo& rhs);

// ===================
// struct SubQueueInfo
// ===================

/// VST representing a sub-queue which will be receiving a message.
struct SubQueueInfo {
    // FRIENDS
    friend bool operator==(const SubQueueInfo& lhs, const SubQueueInfo& rhs);

  private:
    // DATA
    bdlb::BigEndianUint32 d_subQueueId;
    // Id of the sub-queue

    RdaInfo d_rdaInfo;
    // RDA info for the sub-queue

    unsigned char d_reserved[3];
    // Reserved

  public:
    // CREATORS

    /// Create a `SubQueueInfo` with the default sub-queue id and an
    /// unlimited RDA counter.
    SubQueueInfo();

    /// Create a `SubQueueInfo` with the specified `id`.
    SubQueueInfo(unsigned int id);

    /// Create a `SubQueueInfo` with the specified `id` and
    /// `rdaInfo`.
    SubQueueInfo(unsigned int id, const RdaInfo& rdaInfo);

    // MANIPULATORS

    /// Set the sub-queue id to the specified `value` and return a reference
    /// offering modifiable access to this object.
    SubQueueInfo& setId(unsigned int value);

    // ACCESSORS

    /// Return the sub-queue id.
    unsigned int id() const;

    RdaInfo& rdaInfo();
    // Return a reference to the modifiable "RdaInfo" attribute of this object.

    const RdaInfo& rdaInfo() const;
    // Return a reference to the non-modifiable "RdaInfo" attribute of this
    // object.

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
bsl::ostream& operator<<(bsl::ostream& stream, const SubQueueInfo& rhs);

/// Return `true` if `rhs` object contains the value of the same type as
/// contained in `lhs` object and the value itself is the same in both
/// objects, return false otherwise.
bool operator==(const SubQueueInfo& lhs, const SubQueueInfo& rhs);

// ===============
// struct Protocol
// ===============

/// Namespace for protocol generic values and routines
struct Protocol {
    // TYPES

    struct eStopRequestVersion {
        enum Enum { e_V1 = 1, e_V2 = 2 };
    };

    /// A constant used to declare the length of static part of the array of
    /// subQueueIds (or AppKeys).
    static const size_t k_SUBID_ARRAY_STATIC_LEN = 16;

    /// An array of subQueueInfos with statically reserved space for a
    /// number of subQueueInfos (as indicated by the second template
    /// parameter).
    typedef bmqc::Array<SubQueueInfo, k_SUBID_ARRAY_STATIC_LEN>
        SubQueueInfosArray;

    /// An array of subQueueIds with statically reserved space for a number
    /// of subQueueIds (as indicated by the second template parameter).  It
    /// is deprecated by the new SubQueueInfosArray above inside the
    /// brokers, but the SDK still receives and process this older flavor
    /// due to backward compatibility issues.
    typedef bmqc::Array<unsigned int, k_SUBID_ARRAY_STATIC_LEN>
        SubQueueIdsArrayOld;

    /// Holds the client-provided Group Id.
    typedef bsl::string MsgGroupId;

    // CONSTANTS
    static const int k_VERSION = 1;
    // Version of the protocol

    static const int k_DEV_VERSION = 999999;
    // Version assigned to all dev builds.

    static const int k_WORD_SIZE = 4;
    // Number of bytes per word

    static const int k_DWORD_SIZE = 8;
    // Number of bytes per dword

    static const int k_PACKET_MIN_SIZE = 4;
    // Minimum size (bytes) of a packet, needed to know what
    // its real size is.  Must NEVER be changed; used when
    // receiving a packet on the socket.

    static const int k_COMPRESSION_MIN_APPDATA_SIZE = 1024;
    // Threshold below which PUT's message application data
    // will not be compressed regardless of the compression
    // algorithm type set to the PutEventBuilder.

    static const int k_CONSUMER_PRIORITY_INVALID;
    // Constant representing the invalid consumer priority
    // (e.g. of a non-consumer client).

    static const int k_CONSUMER_PRIORITY_MIN;
    // Constant representing the minimum valid consumer
    // priority.  Must be > 'k_CONSUMER_PRIORITY_INVALID'.

    static const int k_CONSUMER_PRIORITY_MAX;
    // Constant representing the maximum valid consumer
    // priority.  Must be >= 'k_CONSUMER_PRIORITY_MIN'.

    static const int k_MSG_GROUP_ID_MAX_LENGTH = 31;
    // Constant representing the maximum valid Group Id size.

    static const int k_MAX_OPTIONS_SIZE;
    // Constant representing the maximum size of options area
    // for any type of event (PUT, PUSH, etc).  Note that
    // each header (PutHeader, PushHeader, etc) may define
    // their own such constant, but we ensure that all such
    // constants have the same value.

    static const unsigned int k_RDA_VALUE_MAX;
    // Constant representing the maximum valid
    // RemainingDeliveryAttempts counter value.

    static const unsigned int k_DEFAULT_SUBSCRIPTION_ID = 0;
    // Internal unique id in Configure request

    // CLASS METHODS

    /// Combine the specified `upper` and `lower` 32 bits to return an
    /// unsigned 64-bit value.
    static bsls::Types::Uint64 combine(unsigned int upper, unsigned int lower);

    /// Return the upper 32 bits of the specified unsigned 64-bit `value`.
    static unsigned int getUpper(bsls::Types::Uint64 value);

    /// Return the lower 32 bits of the specified unsigned 64-bit `value`.
    static unsigned int getLower(bsls::Types::Uint64 value);

    /// Return 48-32 bits of the specified unsigned 64-bit `value`.
    static unsigned short get48to32Bits(bsls::Types::Uint64 value);

    /// Populate the specified `upper` and `lower` buffers with the upper
    /// and lower 32 bits respectively of the specified unsigned 64-bit
    /// `value`.
    static void
    split(unsigned int* upper, unsigned int* lower, bsls::Types::Uint64 value);

    /// Populate the specified `upper` and `lower` buffers with the upper
    /// and lower 32 bits respectively of the specified unsigned 64-bit
    /// `value`.
    static void split(bdlb::BigEndianUint32* upper,
                      bdlb::BigEndianUint32* lower,
                      bsls::Types::Uint64    value);

    /// Populate the specified `upper` and `lower` buffers with the upper 16
    /// and lower 32 bits respectively of the specified `value`.  Behavior
    /// is undefined unless all bits higher than 48 in `value` are zero.
    static void split(bdlb::BigEndianUint16* upper,
                      bdlb::BigEndianUint32* lower,
                      bsls::Types::Uint64    value);
};

// ================
// struct EventType
// ================

/// This struct defines the type of events exchanged between BlazingMQ SDK
/// client and BlazingMQ broker, as well as between BlazingMQ brokers.
struct EventType {
    // TYPES
    enum Enum {
        e_UNDEFINED = 0,
        e_CONTROL   = 1  // Protocol event (schema-based choice)
        ,
        e_PUT                 = 2,
        e_CONFIRM             = 3,
        e_PUSH                = 4,
        e_ACK                 = 5,
        e_CLUSTER_STATE       = 6,
        e_ELECTOR             = 7,
        e_STORAGE             = 8,
        e_RECOVERY            = 9,
        e_PARTITION_SYNC      = 10,
        e_HEARTBEAT_REQ       = 11,
        e_HEARTBEAT_RSP       = 12,
        e_REJECT              = 13,
        e_REPLICATION_RECEIPT = 14
    };

    // CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that an
    /// Event's `type` field is a supported type.
    static const int k_LOWEST_SUPPORTED_EVENT_TYPE = e_CONTROL;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify an
    /// Event's `type` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_EVENT_TYPE = e_REPLICATION_RECEIPT;

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
    /// what constitutes the string representation of a `EventType::Enum`
    /// value.
    static bsl::ostream& print(bsl::ostream&   stream,
                               EventType::Enum value,
                               int             level          = 0,
                               int             spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << EventType::toAscii(EventType::e_CONFIRM);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// CONFIRM
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(EventType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, EventType::Enum value);

// ===================
// struct EncodingType
// ===================

/// This struct defines the types of encoding that can be used by a client
/// or a broker for control messages
struct EncodingType {
    // TYPES
    enum Enum {
        e_UNKNOWN = -1,
        e_BER     = 0  // For backward compatibility, 'BER' needs to be
                       // assigned a value of zero.
        ,
        e_JSON = 1
    };

    // CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that an
    /// Event's `encoding type` field is a supported type.
    static const int k_LOWEST_SUPPORTED_ENCODING_TYPE = e_BER;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify that an
    /// Event's `encoding type` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_ENCODING_TYPE = e_JSON;

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
    /// what constitutes the string representation of a `EncodingType::Enum`
    /// value.
    static bsl::ostream& print(bsl::ostream&      stream,
                               EncodingType::Enum value,
                               int                level          = 0,
                               int                spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << EncodingType::toAscii(EncodingType::e_BER);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// BER
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(EncodingType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, EncodingType::Enum value);

// ======================
// struct EncodingFeature
// ======================

/// This struct defines the field name of the encoding features and the
/// list of supported encoding features exchanged in a Negotiation Message
/// between a broker and its peer (a broker or a client).
struct EncodingFeature {
    // CONSTANTS

    /// Field name of the encoding features
    static const char k_FIELD_NAME[];

    /// BER encoding feature
    static const char k_ENCODING_BER[];

    /// JSON encoding feature
    static const char k_ENCODING_JSON[];
};

/// This struct defines feature names related to High-Availability
struct HighAvailabilityFeatures {
    /// Field name of the encoding features
    static const char k_FIELD_NAME[];

    // CONSTANTS
    static const char k_BROADCAST_TO_PROXIES[];

    static const char k_GRACEFUL_SHUTDOWN[];

    static const char k_GRACEFUL_SHUTDOWN_V2[];
};

/// This struct defines feature names related to MessageProperties
struct MessagePropertiesFeatures {
    /// Field name of the encoding features
    static const char k_FIELD_NAME[];

    // CONSTANTS
    static const char k_MESSAGE_PROPERTIES_EX[];
};

/// TEMPORARILY. This struct defines feature names related to Subscriptions
struct SubscriptionsFeatures {
    /// Field name of the encoding features
    static const char k_FIELD_NAME[];

    // CONSTANTS
    static const char k_CONFIGURE_STREAM[];
};

// =================
// struct OptionType
// =================

/// This struct defines the type of options that can appear in `PUT` or
/// `PUSH` messages.
struct OptionType {
    // TYPES
    enum Enum {
        // Values for the OptionHeader 'type' field.  Valid values are in the
        // range [0, OptionHeader::k_MAX_TYPE].  They must begin at 0 and
        // increment by one.
        e_UNDEFINED         = 0,
        e_SUB_QUEUE_IDS_OLD = 1,
        e_MSG_GROUP_ID      = 2,
        e_SUB_QUEUE_INFOS   = 3
    };

    // CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that an
    /// OptionHeader's `type` field is a supported type.
    static const int k_LOWEST_SUPPORTED_TYPE = e_SUB_QUEUE_IDS_OLD;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify an
    /// OptionHeader's `type` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_TYPE = e_SUB_QUEUE_INFOS;

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
    /// what constitutes the string representation of a `OptionType::Enum`
    /// value.
    static bsl::ostream& print(bsl::ostream&    stream,
                               OptionType::Enum value,
                               int              level          = 0,
                               int              spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << OptionType::toAscii(OptionType::e_UNDEFINED);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// UNDEFINED
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(OptionType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, OptionType::Enum value);

// ==================
// struct EventHeader
// ==================

/// This struct represents the header for all the events received by the
/// broker from local clients or from peer brokers.  A well-behaved event
/// header will always have its fragment bit set to zero.
struct EventHeader {
    // EventHeader structure datagram [8 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |F|                          Length                             |
    //   +---------------+---------------+---------------+---------------+
    //   |PV |   Type    |  HeaderWords  | TypeSpecific  |    Reserved   |
    //   +---------------+---------------+---------------+---------------+
    //       F..: Fragment
    //       PV.: Protocol Version
    //
    //  Fragment (F)..........: Always set to 0
    //  Length................: Total size (bytes) of this event
    //  Protocol Version (PV).: Protocol Version (up to 4 concurrent versions)
    //  Type..................: Type of the event (from EventType::Enum)
    //  HeaderWords...........: Number of words of this event header
    //  TypeSpecific..........: Content specific to the event's type, see below
    //  Reserved..............: For alignment and extension ~ must be 0
    //..
    //
    // TypeSpecific content:
    //: o ControlMessage: represent the encoding used for that control message
    //      |0|1|2|3|4|5|6|7|
    //      +---------------+
    //      |CODEC| Reserved|
    //
    // NOTE: The HeaderWords allows to eventually put event level options
    //       (either by extending the EventHeader struct, or putting new struct
    //       after the EventHeader).  For now, this is left up for future
    //       enhancement as there are no use-case for an event level option.

  private:
    // PRIVATE CONSTANTS
    static const int k_FRAGMENT_NUM_BITS         = 1;
    static const int k_LENGTH_NUM_BITS           = 31;
    static const int k_PROTOCOL_VERSION_NUM_BITS = 2;
    static const int k_TYPE_NUM_BITS             = 6;

    static const int k_FRAGMENT_START_IDX         = 31;
    static const int k_LENGTH_START_IDX           = 0;
    static const int k_PROTOCOL_VERSION_START_IDX = 6;
    static const int k_TYPE_START_IDX             = 0;

    static const int k_FRAGMENT_MASK;
    static const int k_LENGTH_MASK;
    static const int k_PROTOCOL_VERSION_MASK;
    static const int k_TYPE_MASK;

  private:
    // DATA
    bdlb::BigEndianUint32 d_fragmentBitAndLength;
    // Total size (in bytes) of the event (header
    // included), and fragment bit (most significant,
    // always set to zero)

    unsigned char d_protocolVersionAndType;
    // Event type (Control, Push, Pull, etc) and
    // protocol version.

    unsigned char d_headerWords;
    // Number of words in this header struct.

    unsigned char d_typeSpecific;
    // Options and flags specific to this event's type

    BSLA_MAYBE_UNUSED unsigned char d_reserved;
    // Reserved.

  public:
    // PUBLIC CLASS DATA

    /// Maximum size (bytes) of a full event, per protocol limitations.
    static const int k_MAX_SIZE_HARD = (1U << k_LENGTH_NUM_BITS) - 1U;

    /// Maximum size (bytes) of a full event, enforced.
    ///
    /// TBD: The constant `EventHeader::k_MAX_SIZE_SOFT` is used by various
    /// by bmqp builders (including bmqp::StorageEventBuilder::packMessage)
    /// to ensure that the size of the outgoing bmqp event is not bigger
    /// than this constant.  A PUT message is permitted to have a maximum
    /// size of 64MB (see `PutHeader::k_MAX_PAYLOAD_SIZE_SOFT`), and the
    /// replication layer, which uses `bmqp::StorageEventBuilder`, must be
    /// able to build and send that message.  Replication layer adds its own
    /// header and journal record to the PUT message, such that the size of
    /// an outgoing storage message can exceed
    /// `PutHeader::k_MAX_PAYLOAD_SIZE_SOFT`.  So, we assign a value of 65MB
    /// to `StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT`, and assign a value of
    /// at least 66MB to `EventHeader::k_MAX_SIZE_SOFT` such that a PUT
    /// message having the maximum allowable value is processed through the
    /// BlazingMQ pipeline w/o any issues.  The value of
    /// `EventHeader::k_MAX_SIZE_SOFT` is 512Mb to improve batching at high
    /// posting rates.  Also see notes for the
    /// `StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT` constant.
    static const int k_MAX_SIZE_SOFT = 512 * 1024 * 1024;

    /// Highest possible value for the type of an event.
    static const int k_MAX_TYPE = (1 << k_TYPE_NUM_BITS) - 1;

    /// Maximum size (bytes) of an `EventHeader`.
    static const int k_MAX_HEADER_SIZE = ((1 << 8) - 1) *
                                         Protocol::k_WORD_SIZE;

    /// Minimum size (bytes) of an `EventHeader` (that is sufficient to
    /// capture header words).  This value should *never* change.
    static const int k_MIN_HEADER_SIZE = 6;

  public:
    // CREATORS

    /// Create this object with its type set to `EventType::e_UNDEFINED`,
    /// `headerNumWords` and `length` fields set to appropriate value
    /// derived from sizeof() operator and the protocol version set to the
    /// current one.  All other fields are set to zero.
    explicit EventHeader();

    /// Create this object with its type set to the specified `type`, and
    /// `headerNumWords` and `length` fields set to appropriate value
    /// derived from sizeof() operator and the protocol version set to the
    /// current one.  All other fields are set to zero.
    explicit EventHeader(EventType::Enum type);

    // MANIPULATORS

    /// Set the length to the specified `value` (in bytes) and return a
    /// reference offering modifiable access to this object.
    EventHeader& setLength(int value);

    /// Set the protocol version to the specified `value` and return a
    /// reference offering modifiable access to this object.
    EventHeader& setProtocolVersion(unsigned char value);

    /// Set the type to the specified `value` and return a reference
    /// offering modifiable access to this object.
    EventHeader& setType(EventType::Enum value);

    /// Set the number of words composing this header to the specified
    /// `value` and return a reference offering modifiable access to this
    /// object.
    EventHeader& setHeaderWords(unsigned char value);

    /// Set the options and flags specific to this event's type to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    EventHeader& setTypeSpecific(unsigned char value);

    // ACCESSORS

    /// Return the value of the fragment bit of this event.
    int fragmentBit() const;

    /// Return the length of this event (in bytes).
    int length() const;

    /// Return the protocol version set in this event.
    unsigned char protocolVersion() const;

    /// Return the type of this event.
    EventType::Enum type() const;

    /// Return the number of words composing this header.
    unsigned char headerWords() const;

    /// Return the options and flags specific to this event's type.
    unsigned char typeSpecific() const;
};

// ======================
// struct EventHeaderUtil
// ======================

/// This component provides utility methods for `bmqp::EventHeader`.
struct EventHeaderUtil {
  private:
    static const int k_CONTROL_EVENT_ENCODING_NUM_BITS  = 3;
    static const int k_CONTROL_EVENT_ENCODING_START_IDX = 5;
    static const int k_CONTROL_EVENT_ENCODING_MASK;

  public:
    // CLASS METHODS

    /// Set the appropriate bits in the specified `eventHeader` to represent
    /// the specified encoding `type` for a control event.
    static void setControlEventEncodingType(EventHeader*       eventHeader,
                                            EncodingType::Enum type);

    /// Return the encoding type for a control event represented by the
    /// appropriate bits in the specified `eventHeader`.
    static EncodingType::Enum
    controlEventEncodingType(const EventHeader& eventHeader);
};

// ===================
// struct OptionHeader
// ===================

/// This struct represents the header for an option.  In a typical
/// implementation usage, every Option struct will start by an
/// `OptionHeader` member.
struct OptionHeader {
    // OptionHeader structure datagram [4 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |    Type   |P|   TS  |              Words                      |
    //   +---------------+---------------+---------------+---------------+
    //
    //  Type...............: Type of this option
    //  Packed (P).........: Flag to indicate *packed* options.  If set,
    //                       'words' field will be reinterpreted as extra
    //                       type-specific content of this option in addition
    //                       to the 'TS' field, and there will be no option
    //                       content following this header
    //  Type-Specific (TS).: Content specific to this option's type, see below
    //  Words..............: Length (words) of this option, including this
    //                       header.  If *packed*, this field will be
    //                       reinterpreted as additional type-specific content
    //..
    //
    // TypeSpecific content:
    // o e_SUB_QUEUE_INFOS: If *packed*, 'words' field will be reinterpreted as
    //                      the RDA counter for the default SubQueueId.  Note
    //                      that the options is only *packed* for non-fanout
    //                      mode.  Else, 'TS' field will represent the size of
    //                      each item in the SubQueueInfosArray encoded in the
    //                      option content.
    //
    // NOTE:
    //  o In order to preserve alignment, this struct can *NOT* be changed
    //  o Every 'Option' must start by this header, and be 4 bytes aligned with
    //    optional padding at the end.
    //  o For efficiency, since option lookup will be a linear search, options
    //    should be added by decreasing order of usage (or could be sorted by
    //    option id and use optimize the linear search to cut off earlier
    //    eventually).
    //  o Options that are followed by one or multiple 'variable length'
    //    fields may include a byte representing the size (words) of the
    //    optionHeader.
    //
    /// Example of a 'real' option struct:
    /// ---------------------------------
    //..
    //  struct MyStruct {
    //     OptionHeader          d_optionHeader;
    //     bdlb::BigEndianUint32 d_timesamp;
    //     bdlb::BigEndianUint32 d_msgId;
    //   };
    //..

  private:
    // PRIVATE CONSTANTS
    static const int k_TYPE_NUM_BITS          = 6;
    static const int k_PACKED_NUM_BITS        = 1;
    static const int k_TYPE_SPECIFIC_NUM_BITS = 4;
    static const int k_WORDS_NUM_BITS         = 21;

    static const int k_TYPE_START_IDX          = 26;
    static const int k_PACKED_START_IDX        = 25;
    static const int k_TYPE_SPECIFIC_START_IDX = 21;
    static const int k_WORDS_START_IDX         = 0;

    static const int k_TYPE_MASK;
    static const int k_PACKED_MASK;
    static const int k_TYPE_SPECIFIC_MASK;
    static const int k_WORDS_MASK;

  private:
    // DATA
    bdlb::BigEndianUint32 d_content;
    // Content of this OptionHeader, including option
    // type, packed flag, type-specific content and
    // number of words in this option (including this
    // OptionHeader).

  public:
    // PUBLIC CLASS DATA

    /// Highest possible value for the type of an option.
    static const int k_MAX_TYPE = (1 << k_TYPE_NUM_BITS) - 1;

    /// Maximum size (bytes) of an option, including the header.
    static const int k_MAX_SIZE = ((1 << k_WORDS_NUM_BITS) - 1) *
                                  Protocol::k_WORD_SIZE;

    /// Minimum size (bytes) of an `OptionHeader` (that is sufficient to
    /// capture header words).  This value should *never* change.
    static const int k_MIN_HEADER_SIZE = 4;

  public:
    // CREATORS

    /// Create this object with its type set to `OptionType::e_UNDEFINED`,
    /// packed flag set to false, `type-specific` field set to zeros and
    /// `words` field set to size of this object, which is derived from
    /// `sizeof()` operator.
    explicit OptionHeader();

    /// Create this object with its type set to the specified `type`,
    /// packed flag set to the specified `isPacked` and `type-specific`
    /// field set to zeros.  If `isPacked`, `words` field is set to zeros;
    /// else, `words` field is set to size of this object, that is derived
    /// from `sizeof()` operator.
    explicit OptionHeader(OptionType::Enum type, bool isPacked);

    // MANIPULATORS

    /// Set the type to the specified `value` and return a reference
    /// offering modifiable access to this object.
    OptionHeader& setType(OptionType::Enum value);

    /// Set the packed flag to the specified `value` and return a reference
    /// offering modifiable access to this object.
    OptionHeader& setPacked(bool value);

    /// Set the type-specific content to the specified `value` and return a
    /// reference offering modifiable access to this object.
    OptionHeader& setTypeSpecific(unsigned char value);

    /// Set the total size of this option (header included), to the
    /// specified `value` (in words), and return a reference offering
    /// modifiable access to this object.  If *packed*, this is
    /// reinterpreted as additional type-specific content.
    OptionHeader& setWords(int value);

    // ACCESSORS

    /// Return the type of this option.
    OptionType::Enum type() const;

    /// Return the packed flag of this option.
    bool packed() const;

    /// Return the type-specific content of this option.
    unsigned char typeSpecific() const;

    /// Return the number of words composing this option, header included.
    /// If *packed*, this is reinterpreted as additional type-specific
    /// content.
    int words() const;
};

// ==============================
// struct MessagePropertiesHeader
// ==============================

/// This struct represents the header for message properties area in a PUT
/// or PUSH message.  This header will be followed by one or more message
/// properties.
struct MessagePropertiesHeader {
    // MessagePropertiesHeader structure datagram [6 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   | R |PHS2X| HS2X|   MPAW Upper  |          MPAW Lower           |
    //   +---------------+---------------+---------------+---------------+
    //   |   Reserved    |    NumProps   |
    //   +---------------+---------------+
    //
    //       R......: Reserved ~ must be 0
    //       PHS2X..: 'MessagePropertyHeader' size as 2byte multiple.
    //       HS2X...: Header size as 2byte multiple.
    //       MPAW...: Message properties area words
    //
    //  MessagePropertyHeader size..: Size (as a multiple of 2) of a
    //                                'MessagePropertyHeader'.  This size is
    //                                captured in this struct instead of
    //                                'MessagePropertyHeader' itself so as
    //                                to keep later's size to a minimum.  Also
    //                                note that this size is captured as a
    //                                multiple of 2, not 4, because the size of
    //                                'MessagePropertyHeader' may not be a
    //                                multiple of 4.
    //  Header Size.................: Total size (as a multiple of 2) of this
    //                                header.  Note that this size is captured
    //                                as a multiple of 2, not 4, because its
    //                                size may not be a multiple of 4.
    //  MessagePropertiesAreaWords..: Total size (words) of the message
    //                                properties area, including this
    //                                'MessagePropertiesHeader', including any
    //                                other sub-header, and including all
    //                                message properties with any necessary
    //                                padding.
    //  Num Properties (NumProps)...: Total number of properties in the message
    //..
    //
    // Alignment requirement: 2 bytes.
    //
    // Following this header, one or more message properties will be present.
    // The layout of the entire message properties area is described below:
    //..
    //  [MessagePropertiesH][MessagePropertyH #1][MessagePropertyH #2]...
    //  [MessagePropertyH #n][PropName #1][PropValue #1][PropName #2]
    //  [PropValue #2]...[PropName #n][PropValue #n][word alignment padding]
    //..
    //
    // To describe the layout in words, MessagePropertiesHeader is followed by
    // the MessagePropertyHeader's of *all* properties, which is then followed
    // by a sequence of pairs of (PropertyName, PropertyValue).
    //
    // Note that the alignment requirement for MessagePropertyHeader is 2-byte
    // boundary, and there is no alignment requirement on the property name or
    // value fields.  In order to avoid unnecessary padding after each pair of
    // property name and value (so as to fulfil the alignment requirement of
    // the succeeding MessagePropertyHeader), all MessagePropertyHeader's are
    // put first, a layout which doesn't any padding b/w the headers.
    //
    // Also note that the entire message properties area needs to be word
    // aligned, because of 'MessagePropertiesHeader.MessagePropertiesAreaWords'
    // field.  The payload following message properties area does not have any
    // alignment requirement though.

  private:
    // PRIVATE CONSTANTS
    static const int k_MSG_PROPS_AREA_WORDS_LOWER_NUM_BITS = 16;
    static const int k_MSG_PROPS_AREA_WORDS_UPPER_NUM_BITS = 8;
    static const int k_MSG_PROPS_AREA_WORDS_NUM_BITS =
        k_MSG_PROPS_AREA_WORDS_LOWER_NUM_BITS +
        k_MSG_PROPS_AREA_WORDS_UPPER_NUM_BITS;
    static const int k_NUM_PROPERTIES_NUM_BITS = 8;

    static const int k_HEADER_SIZE_2X_NUM_BITS = 3;
    static const int k_MPH_SIZE_2X_NUM_BITS    = 3;
    static const int k_RESERVED_NUM_BITS       = 2;

    static const int k_HEADER_SIZE_2X_START_IDX = 0;
    static const int k_MPH_SIZE_2X_START_IDX    = 3;

    static const int k_HEADER_SIZE_2X_MASK;
    static const int k_MPH_SIZE_2X_MASK;

  private:
    // DATA
    char d_mphSize2xAndHeaderSize2x;

    unsigned char d_msgPropsAreaWordsUpper;

    bdlb::BigEndianUint16 d_msgPropsAreaWordsLower;

    BSLA_MAYBE_UNUSED char d_reserved;

    unsigned char d_numProperties;

  public:
    // PUBLIC CLASS DATA

    /// Maximum size (bytes)
    static const int k_MAX_MESSAGE_PROPERTIES_SIZE =
        ((1 << k_MSG_PROPS_AREA_WORDS_NUM_BITS) - 1) * Protocol::k_WORD_SIZE;

    /// Maximum size (bytes) of a `MessagePropertiesHeader`.
    static const int k_MAX_HEADER_SIZE = ((1 << k_HEADER_SIZE_2X_NUM_BITS) -
                                          1) *
                                         2;

    /// Minimum size (bytes) of a `MessagePropertiesHeader` that is
    /// sufficient to capture header words.  This value should *never*
    /// change.
    static const int k_MIN_HEADER_SIZE = 1;

    /// Maximum size (bytes) of a `MessagePropertyHeader`.
    static const int k_MAX_MPH_SIZE = ((1 << k_MPH_SIZE_2X_NUM_BITS) - 1) * 2;

    static const int k_MAX_NUM_PROPERTIES = (1 << k_NUM_PROPERTIES_NUM_BITS) -
                                            1;

  public:
    // CREATORS

    /// Create this object where `headerSize` and
    /// `messagePropertiesAreaWords` fields are set to size of this object,
    /// the value of which is directly derived from `sizeof()` operator,
    /// the `mphSize` field is set to the size of `messagePropertyHeader`
    /// object, the value of which is directly derived from `sizeof()`
    /// operator.  All other fields are set to zero.
    MessagePropertiesHeader();

    // MANIPULATORS
    MessagePropertiesHeader& setHeaderSize(int value);

    MessagePropertiesHeader& setMessagePropertyHeaderSize(int value);

    MessagePropertiesHeader& setMessagePropertiesAreaWords(int value);

    MessagePropertiesHeader& setNumProperties(int value);

    // ACCESSORS
    int headerSize() const;

    int messagePropertyHeaderSize() const;

    int messagePropertiesAreaWords() const;

    int numProperties() const;
};

// ============================
// struct MessagePropertyHeader
// ============================

/// This struct represents the header for a message property in a PUT or
/// PUSH message.
struct MessagePropertyHeader {
    // MessagePropertyHeader struct datagram [6 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |R|PropType | PropValueLenUpper |       PropValueLenLower       |
    //   +---------------+---------------+---------------+---------------+
    //   |   R2  |     PropNameLen       |
    //   +---------------+---------------+
    //       R...: Reserved
    //       R2..: Reserved (2nd set of bits)
    //
    //  PropType...........: Data type of the message property
    //  PropValueLenUpper..: Upper 10 bits of the field capturing length of the
    //                       property value.
    //  PropValueLenLower..: Lower 16 bits of the field capturing length of the
    //                       property value.
    //  PropNameLen........: Length of the property name.
    //  Reserved...........: For alignment and extension ~ must be 0
    //..
    //
    // This struct must be 2-byte aligned.  See comments in
    // 'MessagePropertiesHeader' for details.

  private:
    // PRIVATE CONSTANTS
    static const int k_RESERVED1_NUM_BITS            = 1;
    static const int k_PROP_TYPE_NUM_BITS            = 5;
    static const int k_PROP_VALUE_LEN_UPPER_NUM_BITS = 10;
    static const int k_PROP_VALUE_LEN_LOWER_NUM_BITS = 16;
    static const int k_PROP_VALUE_LEN_NUM_BITS =
        k_PROP_VALUE_LEN_UPPER_NUM_BITS + k_PROP_VALUE_LEN_LOWER_NUM_BITS;
    static const int k_RESERVED2_NUM_BITS     = 4;
    static const int k_PROP_NAME_LEN_NUM_BITS = 12;

    static const int k_RESERVED1_START_IDX            = 15;
    static const int k_PROP_TYPE_START_IDX            = 10;
    static const int k_PROP_VALUE_LEN_UPPER_START_IDX = 0;
    static const int k_RESERVED2_START_IDX            = 12;
    static const int k_PROP_NAME_LEN_START_IDX        = 0;

    static const int k_PROP_TYPE_MASK;
    static const int k_PROP_VALUE_LEN_UPPER_MASK;
    static const int k_PROP_NAME_LEN_MASK;

  private:
    // DATA
    bdlb::BigEndianUint16 d_propTypeAndPropValueLenUpper;

    bdlb::BigEndianUint16 d_propValueLenLower;

    bdlb::BigEndianUint16 d_reservedAndPropNameLen;

  public:
    // PUBLIC CONSTANTS
    static const int k_MAX_PROPERTY_VALUE_LENGTH =
        (1 << k_PROP_VALUE_LEN_NUM_BITS) - 1;

    static const int k_MAX_PROPERTY_NAME_LENGTH =
        (1 << k_PROP_NAME_LEN_NUM_BITS) - 1;

  public:
    // CREATORS

    /// Create this object with all fields set to zero.
    MessagePropertyHeader();

    // MANIPULATORS
    MessagePropertyHeader& setPropertyType(int value);

    MessagePropertyHeader& setPropertyValueLength(int value);

    MessagePropertyHeader& setPropertyNameLength(int value);

    // ACCESSORS
    int propertyType() const;

    int propertyValueLength() const;

    int propertyNameLength() const;
};

struct SchemaWireId {
  public:
    static const unsigned k_MAX_VALUE = 0xffff;

  private:
    bdlb::BigEndianUint16 d_value;

  public:
    // CREATORS
    SchemaWireId();

  public:
    // PUBLIC MODIFIERS
    void set(unsigned value);

    // PUBLIC ACCESSORS
    unsigned value() const;
};

// ================
// struct PutHeader
// ================

/// This struct represents the header for a `PUT` event.  A `PUT` event is
/// the event sent by a client to the broker to post message on queue(s).
struct PutHeader {
    // PutHeader structure datagram [36 bytes (followed by zero or more options
    //                                         then zero or more message
    //                                         properties, then data payload)]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |   F   |                     MessageWords                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                 OptionsWords                  | CAT |   HW    |
    //   +---------------+---------------+---------------+---------------+
    //   |                            QueueId                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                  MessageGUID or CorrelationId                 |
    //   +---------------+---------------+---------------+---------------+
    //   |                  MessageGUID (cont.) or Null                  |
    //   +---------------+---------------+---------------+---------------+
    //   |                  MessageGUID (cont.) or Null                  |
    //   +---------------+---------------+---------------+---------------+
    //   |                  MessageGUID (cont.) or Null                  |
    //   +---------------+---------------+---------------+---------------+
    //   |                            CRC32-C                            |
    //   +---------------+---------------+---------------+---------------+
    //   |           SchemaId            |           Reserved            |
    //   +---------------+---------------+---------------+---------------+
    //       F..: Flags
    //       CAT: CompressionAlgorithmType
    //       HW.: HeaderWords
    //
    //  Flags (F)..................: Flags. See PutHeaderFlags struct.
    //  MessageWords...............: Total size (words) of the message,
    //                               including this header, the options (header
    //                               + content) and application data
    //  OptionsWords...............: Total size (words) of the options area
    //  CAT........................: Compression Algorithm Type
    //  HeaderWords (HW)...........: Total size (words) of this MessageHeader
    //  QueueId....................: Id of the queue (as announced during open
    //                               queue by the producer of this Put event)
    //  MessageGUID/CorrelationId..: Depending upon context, this field is
    //                               either MessageGUID, or CorrelationId
    //                               specified by the source of 'PUT' message
    //                               Note that correlationId is always limited
    //                               to 24 bits.
    //  CRC32-C....................: CRC32-C calculated over the application
    //                               data.
    //  SchemaId...................: Binary protocol representation of the
    //                               Schema Id associated with subsequent
    //                               MessageProperties.
    //..
    //
    // Following this header, zero or more options may be present.  They are
    // aligned consecutively in memory, and all start with an OptionHeader,
    // like so:
    //..
    //   [OptionHeader..(OptionContent)][OptionHeader..(OptionContent)]
    //..
    //
    // Options are followed by message properties, which are also optional.
    // One of the 'PutHeaderFlags' indicates the presence of message
    // properties.  Message properties area must be word aligned (each message
    // property need not be word aligned).
    //
    // Message properties are followed by the data payload.  Data must be word
    // aligned.
    //
    // The combination of message properties and data payload may be referred
    // to as 'application data'.
    //
    // From the first byte of the PutHeader, application data can be found by:
    //..
    //     ptr += 4 * (headerWords + optionsWords)
    //..

  private:
    // FRIENDS
    friend struct PutHeaderFlags;
    // So that it can access the 'k_FLAGS_NUM_BITS' value.

    // PRIVATE CONSTANTS
    static const int k_FLAGS_NUM_BITS          = 4;
    static const int k_MSG_WORDS_NUM_BITS      = 28;
    static const int k_OPTIONS_WORDS_NUM_BITS  = 24;
    static const int k_CAT_NUM_BITS            = 3;
    static const int k_HEADER_WORDS_NUM_BITS   = 5;
    static const int k_CORRELATION_ID_NUM_BITS = 24;
    static const int k_CORRELATION_ID_LEN      = 3;  // In bytes

    static const int k_FLAGS_START_IDX         = 28;
    static const int k_MSG_WORDS_START_IDX     = 0;
    static const int k_OPTIONS_WORDS_START_IDX = 8;
    static const int k_CAT_START_IDX           = 5;
    static const int k_HEADER_WORDS_START_IDX  = 0;

    static const int k_FLAGS_MASK;
    static const int k_MSG_WORDS_MASK;
    static const int k_OPTIONS_WORDS_MASK;
    static const int k_CAT_MASK;
    static const int k_HEADER_WORDS_MASK;

    // DATA
    bdlb::BigEndianUint32 d_flagsAndMessageWords;
    // Flags, and total size (in words) of the message
    // (including this header, options and message
    // payload); see 'PutHeaderFlags' struct for the
    // meaning of this field.

    bdlb::BigEndianUint32 d_optionsAndHeaderWordsAndCAT;
    // Total size (in words) of options area, size (in
    // words) of this header and Compression Algorithm
    // type.

    bdlb::BigEndianInt32 d_queueId;
    // Queue Id.

    unsigned char d_guidOrCorrId[bmqt::MessageGUID::e_SIZE_BINARY];
    // MessageGUID/CorrelationId associated with this
    // header.  Note that the 24-bit correlationId is
    // stored in d_guidOrCorrId[1-3].

    bdlb::BigEndianUint32 d_crc32c;
    // CRC32-C calculated over the application data
    // associated with this header.

    SchemaWireId d_schemaId;

    BSLA_MAYBE_UNUSED unsigned char d_reserved[2];
    // Reserved.

  public:
    // PUBLIC CLASS DATA

    /// Maximum size (bytes) of a full PutMessage with header and payload.
    static const int k_MAX_SIZE = ((1 << k_MSG_WORDS_NUM_BITS) - 1) *
                                  Protocol::k_WORD_SIZE;

    /// Maximum size (bytes) of a PutMessage payload (without header or
    /// options, ...).
    ///
    /// Also see notes in `EventHeader::k_MAX_SIZE_SOFT` and
    /// `StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT`.
    ///
    /// NOTE: the protocol supports up to k_MAX_SIZE for the entire
    /// PutMessage with payload, but the PutEventBuilder only validates the
    /// payload size against this size; so this k_MAX_PAYLOAD_SIZE_SOFT can
    /// be increased but not up to `k_MAX_SIZE`.
    static const int k_MAX_PAYLOAD_SIZE_SOFT = 64 * 1024 * 1024;

    static const int k_MAX_SIZE_SOFT = (64 + 2) * 1024 * 1024;
    /// Maximum size (bytes) of the options area.
    static const int k_MAX_OPTIONS_SIZE = ((1 << k_OPTIONS_WORDS_NUM_BITS) -
                                           1) *
                                          Protocol::k_WORD_SIZE;

    /// Maximum size (bytes) of a `PutHeader`.
    static const int k_MAX_HEADER_SIZE = ((1 << k_HEADER_WORDS_NUM_BITS) - 1) *
                                         Protocol::k_WORD_SIZE;

    /// Minimum size (bytes) of a `PutHeader` (that is sufficient to
    /// capture header words).  This value should *never* change.
    static const int k_MIN_HEADER_SIZE = 8;

    /// Minimum size (bytes) of a `PutHeader` that contains SchemaId.
    static const int k_MIN_HEADER_SIZE_FOR_SCHEMA_ID = 36;

    // PUBLIC TYPES
    typedef struct PutHeaderFlags    Flags;
    typedef struct PutHeaderFlagUtil FlagUtil;

  public:
    // CREATORS

    /// Create this object where `headerWords` and `messageWords` fields
    /// are set to size of this object, the value for which is directly
    /// derived from `sizeof()` operator.  All other fields are set to zero.
    PutHeader();

    // MANIPULATORS

    /// Set the flags mask to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PutHeader& setFlags(int value);

    /// Set the number of words of this entire event (including this header,
    /// the options and the message payload) to the specified `value` and
    /// return a reference offering modifiable access to this object.
    PutHeader& setMessageWords(int value);

    /// Set the number of words of the options area of this event to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    PutHeader& setOptionsWords(int value);

    /// Set the compression algorithm type of this event to the specified
    /// `value` and return a reference offering modifiable access to this
    /// object.
    PutHeader&
    setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::Enum value);

    /// Set the number of words of the header of this message to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    PutHeader& setHeaderWords(int value);

    /// Set the queue id to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PutHeader& setQueueId(int value);

    /// Set the correlation id to the specified `value` and return a
    /// reference offering modifiable access to this object.  Note that the
    /// previous MessageGUID or correlationId value, if any, will be
    /// overwritten.
    PutHeader& setCorrelationId(int value);

    /// Set the message GUID to the specified `value`, and return a
    /// reference offering modifiable access to this object.  Note that the
    /// previous MessageGUID or correlationId value, if any, will be
    /// overwritten.
    PutHeader& setMessageGUID(const bmqt::MessageGUID& value);

    /// Set the CRC32-C to the specified `value` and return a reference
    /// offering modifiable access to this object.  Note that the previous
    /// CRC32-C, if any, will be overwritten.
    PutHeader& setCrc32c(unsigned int value);

    /// Return a reference offering modifiable access to Schema Id
    /// associated with this object.
    SchemaWireId& schemaId();

    // ACCESSORS

    /// Return the flags mask of this header.
    int flags() const;

    /// Return the number of words of this entire event.
    int messageWords() const;

    /// Return the number of words of the options area of this event.
    int optionsWords() const;

    /// Return the compression algorithm type of this event.
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType() const;

    /// Return the number of words of the header of this event.
    int headerWords() const;

    /// Return the queue id associated to this event.
    int queueId() const;

    /// Return the correlation id associated to this event.
    int correlationId() const;

    /// Return the message guid associated with this object.
    const bmqt::MessageGUID& messageGUID() const;

    /// Return the CRC32-C associated with this object.
    unsigned int crc32c() const;

    /// Return the binary protocol representation of Schema Id.
    const SchemaWireId& schemaId() const;
};

// =====================
// struct PutHeaderFlags
// =====================

/// This struct defines the meanings of each bits of the flags field of the
/// `PutHeader` structure.
struct PutHeaderFlags {
    // TYPES
    enum Enum {
        e_ACK_REQUESTED = (1 << 0)  // Ack for PUT msg is requested
        ,
        e_MESSAGE_PROPERTIES = (1 << 1)  // Contains message properties
        ,
        e_UNUSED3 = (1 << 2),
        e_UNUSED4 = (1 << 3)
    };

    // PUBLIC CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that a
    /// PutHeader's `Flags` field is a supported type.
    static const int k_LOWEST_SUPPORTED_PUT_FLAG = e_ACK_REQUESTED;

    /// NOTE: This value must always be equal to the highest *supported*
    /// type in the enum because it is being used to verify a PutHeader's
    /// `Flags` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_PUT_FLAG = e_MESSAGE_PROPERTIES;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify a
    /// PutHeader's `Flags` field is a valid type.
    static const int k_HIGHEST_PUT_FLAG = e_UNUSED4;

    /// Used by test drivers
    static const int k_VALUE_COUNT = PutHeader::k_FLAGS_NUM_BITS;

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
    static bsl::ostream& print(bsl::ostream&        stream,
                               PutHeaderFlags::Enum value,
                               int                  level          = 0,
                               int                  spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(PutHeaderFlags::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(PutHeaderFlags::Enum*    out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, PutHeaderFlags::Enum value);

// ========================
// struct PutHeaderFlagUtil
// ========================

/// This component provides utility methods for `bmqp::PutHeaderFlags`.
struct PutHeaderFlagUtil {
    // CLASS METHODS

    /// Set the specified `flag` to true in the bit-mask in the specified
    /// `flags`.
    static void setFlag(int* flags, PutHeaderFlags::Enum flag);

    /// Resets the specified `flag` to false in the bit-mask in the
    /// specified `flags`.
    static void unsetFlag(int* flags, PutHeaderFlags::Enum flag);

    /// Return true if the bit-mask in the specified `flags` has the
    /// specified `flag` set, or false if not.
    static bool isSet(int flags, PutHeaderFlags::Enum flag);

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

// ================
// struct AckHeader
// ================

/// This struct represents header for an `ACK` event.  An `ACK` event is the
/// event sent by the broker to a client in response to a post message on
/// queue.  Such event is optional, depending on flags used at queue open.
struct AckHeader {
    // AckHeader structure datagram [4 bytes (followed by one or multiple
    //                                            AckMessage)]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |  HW   |  PMW  |     Flags     |           Reserved            |
    //   +---------------+---------------+---------------+---------------+
    //       HW..: HeaderWords
    //       PMW.: PerMessageWords
    //       R...: Reserved
    //
    //  HeaderWords (HW)......: Total size (words) of this AckHeader
    //  PerMessageWords (PMW).: Size (words) used for each AckMessage in the
    //                          payload following this AckHeader
    //  Flags.................: bitmask of flags specifying this header
    //                          see AckHeaderFlags struct
    //  Reserved..............: For alignment and extension ~ must be 0
    //..

  private:
    // PRIVATE CONSTANTS
    static const int k_HEADER_WORDS_NUM_BITS  = 4;
    static const int k_PER_MSG_WORDS_NUM_BITS = 4;

    static const int k_HEADER_WORDS_START_IDX  = 4;
    static const int k_PER_MSG_WORDS_START_IDX = 0;

    static const int k_HEADER_WORDS_MASK;
    static const int k_PER_MSG_WORDS_MASK;

    // DATA
    unsigned char d_headerWordsAndPerMsgWords;
    // Total size (words) of this header and number of words of
    // each AckMessage in the payload that follows.

    unsigned char d_flags;
    // Bitmask of flags.

    BSLA_MAYBE_UNUSED unsigned char d_reserved[2];
    // Reserved.

  public:
    // PUBLIC CLASS DATA

    /// Maximum size (bytes) of an `AckHeader`.
    static const int k_MAX_HEADER_SIZE = ((1 << k_HEADER_WORDS_NUM_BITS) - 1) *
                                         Protocol::k_WORD_SIZE;

    /// Maximum size (bytes) of an `AckMessage`.
    static const int k_MAX_PERMESSAGE_SIZE = ((1 << k_PER_MSG_WORDS_NUM_BITS) -
                                              1) *
                                             Protocol::k_WORD_SIZE;

    /// Minimum size (bytes) of an `AckHeader` (which is sufficient to
    /// capture header words).  This value should *never* change.
    static const int k_MIN_HEADER_SIZE = 1;

  public:
    // CREATORS

    /// Create this object where all fields except `headerWords` and
    /// `perMessageWords` are set to zero.  Field `headerWords` is set to
    /// size of this object, the value for which is directly derived from
    /// `sizeof()` operator; and field `perMessageWords` is set to the size
    /// of the `AckMessage` struct.
    explicit AckHeader();

    // MANIPULATORS

    /// Set the number of words of this header to the specified `value` and
    /// return a reference offering modifiable access to this object.
    AckHeader& setHeaderWords(int value);

    /// Set the number of words of each AckMessage in the payload that
    /// follows this header to the specified `value` and return a reference
    /// offering modifiable access to this object.
    AckHeader& setPerMessageWords(int value);

    /// Set the flags mask of the flags field of this header to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    AckHeader& setFlags(unsigned char value);

    // ACCESSORS

    /// Return the number of words of this header.
    int headerWords() const;

    /// Return the number of words of each AckMessage in the payload that
    /// follows.
    int perMessageWords() const;

    /// Return the flags mask of this header.
    unsigned char flags() const;
};

// =====================
// struct AckHeaderFlags
// =====================

/// This struct defines the meanings of each bits of the flags field of the
/// `AckHeader` structure.
struct AckHeaderFlags {
    // TYPES
    enum Enum {
        e_UNUSED1 = (1 << 0),
        e_UNUSED2 = (1 << 1),
        e_UNUSED3 = (1 << 2),
        e_UNUSED4 = (1 << 4),
        e_UNUSED5 = (1 << 4),
        e_UNUSED6 = (1 << 5),
        e_UNUSED7 = (1 << 6),
        e_UNUSED8 = (1 << 7)
    };
};

// =================
// struct AckMessage
// =================

/// This struct defines the (repeated) payload following the `AckHeader`
/// struct.
struct AckMessage {
    // AckMessage structure datagram [24 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |   R   |Status |            CorrelationId                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                          MessageGUID                          |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                            QueueId                            |
    //   +---------------+---------------+---------------+---------------+
    //       R.: Reserved
    //
    //  Reserved (R)..: For alignment and extension ~ must be 0
    //  Status........: Status of this message.
    //  CorrelationId.: Id specified by the producer client to correlate this
    //                  message
    //  MessageGUID...: MessageGUID associated to this message by the broker
    //  QueueId.......: Id of the queue (as advertised during open queue by the
    //                  producer of the Put event this Ack is a response of)
    //..

  private:
    // PRIVATE CONSTANTS
    static const int k_STATUS_NUM_BITS = 4;
    static const int k_CORRID_NUM_BITS = 24;

    static const int k_STATUS_START_IDX = 24;
    static const int k_CORRID_START_IDX = 0;

    static const int k_STATUS_MASK;
    static const int k_CORRID_MASK;

    // DATA
    bdlb::BigEndianInt32 d_statusAndCorrelationId;
    // Status code and associated correlation Id.

    unsigned char d_messageGUID[bmqt::MessageGUID::e_SIZE_BINARY];
    // MessageGUID associated to this message.

    bdlb::BigEndianInt32 d_queueId;
    // Queue Id.

  public:
    // PUBLIC CLASS DATA

    /// Highest possible value for the status.
    static const int k_MAX_STATUS = (1 << k_STATUS_NUM_BITS) - 1;

    /// Constant to indicate no correlation Id.
    static const int k_NULL_CORRELATION_ID = 0;

  public:
    // CREATORS

    /// Create this object where all fields are set to zero.
    AckMessage();

    /// Create an instance with the specified `status`, `correlationId`,
    /// `guid`, and `queueId`.
    AckMessage(int                      status,
               int                      correlationId,
               const bmqt::MessageGUID& guid,
               int                      queueId);

    // MANIPULATORS

    /// Set the status of this message to the specified `value` and return a
    /// reference offering modifiable access to this object.
    AckMessage& setStatus(int value);

    /// Set the correlation Id to the specified `value` and return a
    /// reference offering modifiable access to this object.
    AckMessage& setCorrelationId(int value);

    /// Set the messageGUID of this message to the specified `value` and
    /// return a reference offering modifiable access to this object.
    AckMessage& setMessageGUID(const bmqt::MessageGUID& value);

    /// Set the queue id to the specified `value` and return a reference
    /// offering modifiable access to this object.
    AckMessage& setQueueId(int value);

    // ACCESSORS

    /// Return the status of this message.
    int status() const;

    /// Return the correlation Id of this message.
    int correlationId() const;

    /// Return the messageGUID of this message.
    const bmqt::MessageGUID& messageGUID() const;

    /// Return the queue id associated to this message.
    int queueId() const;
};

// =================
// struct PushHeader
// =================

/// This struct represents the header for a `PUSH` event.  A `PUSH` event is
/// the event sent by the broker to a client to deliver a message from
/// queue(s).
struct PushHeader {
    // PushHeader structure datagram [32 bytes (followed by options, if any,
    //                                          then data payload)]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |   F   |                     MessageWords                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                 OptionsWords                  | CAT |   HW    |
    //   +---------------+---------------+---------------+---------------+
    //   |                            QueueId                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                          MessageGUID                          |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |           SchemaId            |           Reserved            |
    //   +---------------+---------------+---------------+---------------+
    //       F..: Flags
    //       CAT: CompressionAlgorithmType
    //       HW.: HeaderWords
    //
    //  Flags (F)........: Flags associated to this message.  See
    //                     PushHeaderFlags struct
    //  MessageWords.....: Total size (words) of the message, including this
    //                     header, the options (header + content) and the
    //                     message payload
    //  OptionsWords.....: Total size (words) of the options area
    //  CAT..............: Compression Algorithm Type
    //  HeaderWords (HW).: Total size (words) of this PushHeader
    //  QueueId..........: Id of the queue (as advertised during open queue by
    //                     the consumer of this Push event)
    //  MessageGUID......: MessageGUID associated to this message by the broker
    //..
    //
    // NOTE: From the first byte of the PushHeader the payload can be found by:
    //       ptr +=  4 * (headerWords + optionsWords).  Note that if
    //       'bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD' flag is set, then
    //       payload is not present.
    //
    // Following this header, each options are aligned consecutively in memory,
    // and all start with an OptionHeader.
    //   [OptionHeader..(OptionContent)][OptionHeader..(OptionContent)]
    //
    // The (optional) data payload then follows this header.  If present, data
    // must be padded.

  private:
    // FRIENDS
    friend struct PushHeaderFlags;
    // So that it can access the 'k_FLAGS_NUM_BITS' value.

    // PRIVATE CONSTANTS
    static const int k_FLAGS_NUM_BITS         = 4;
    static const int k_MSG_WORDS_NUM_BITS     = 28;
    static const int k_OPTIONS_WORDS_NUM_BITS = 24;
    static const int k_CAT_NUM_BITS           = 3;
    static const int k_HEADER_WORDS_NUM_BITS  = 5;

    static const int k_FLAGS_START_IDX         = 28;
    static const int k_MSG_WORDS_START_IDX     = 0;
    static const int k_OPTIONS_WORDS_START_IDX = 8;
    static const int k_CAT_START_IDX           = 5;
    static const int k_HEADER_WORDS_START_IDX  = 0;

    static const int k_FLAGS_MASK;
    static const int k_MSG_WORDS_MASK;
    static const int k_OPTIONS_WORDS_MASK;
    static const int k_CAT_MASK;
    static const int k_HEADER_WORDS_MASK;

    // DATA
    bdlb::BigEndianUint32 d_flagsAndMessageWords;
    // Flags, and total size (in words) of the message
    // (including this header, options and message
    // payload); see 'PushHeaderFlags' struct for the
    // meaning of this field.

    bdlb::BigEndianUint32 d_optionsAndHeaderWordsAndCAT;
    // Total size (in words) of options area, size (in
    // words) of this header and Compression Algorithm
    // type.

    bdlb::BigEndianInt32 d_queueId;
    // Queue Id.

    unsigned char d_messageGUID[bmqt::MessageGUID::e_SIZE_BINARY];
    // MessageGUID associated to this message by the
    // broker.

    SchemaWireId d_schemaId;

    BSLA_MAYBE_UNUSED unsigned char d_reserved[2];
    // Reserved.
  public:
    // PUBLIC CLASS DATA

    /// Maximum size (bytes) of a full PushMessage with header and payload.
    static const int k_MAX_SIZE = ((1 << k_MSG_WORDS_NUM_BITS) - 1) *
                                  Protocol::k_WORD_SIZE;

    /// Maximum size (bytes) of a PushMessage payload (without header or
    /// options, ...).
    ///
    /// NOTE: the protocol supports up to k_MAX_SIZE for the entire
    /// PushMessage with payload, but the PushEventBuilder only validates
    /// the payload size against this size; so this k_MAX_PAYLOAD_SIZE_SOFT
    /// can be increased but not up to `k_MAX_SIZE`.
    static const int k_MAX_PAYLOAD_SIZE_SOFT = 64 * 1024 * 1024;

    /// Maximum size (bytes) of the options area.
    static const int k_MAX_OPTIONS_SIZE = ((1 << k_OPTIONS_WORDS_NUM_BITS) -
                                           1) *
                                          Protocol::k_WORD_SIZE;

    /// Maximum size (bytes) of a `PushHeader`.
    static const int k_MAX_HEADER_SIZE = ((1 << k_HEADER_WORDS_NUM_BITS) - 1) *
                                         Protocol::k_WORD_SIZE;

    /// Minimum size (bytes) of an `PushHeader` (that is sufficient to
    /// capture header words).  This value should *never* change.
    static const int k_MIN_HEADER_SIZE = 8;

    /// Minimum size (bytes) of a `PushHeader` that contains SchemaId.
    static const int k_MIN_HEADER_SIZE_FOR_SCHEMA_ID = 32;

    // PUBLIC TYPES
    typedef struct PushHeaderFlags    Flags;
    typedef struct PushHeaderFlagUtil FlagUtil;

  public:
    // CREATORS

    /// Create this object where `headerWords` and `messageWords` fields are
    /// set to size of this object, the value for which is directly derived
    /// from `sizeof()` operator.  All other fields are set to zero.
    PushHeader();

    // MANIPULATORS

    /// Set the flags mask to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PushHeader& setFlags(int value);

    /// Set the number of words of this entire event (including this header,
    /// the options and the message payload) to the specified `value` and
    /// return a reference offering modifiable access to this object.
    PushHeader& setMessageWords(int value);

    /// Set the number of words of the options area of this event to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    PushHeader& setOptionsWords(int value);

    /// Set the compression algorithm type of this event to the specified
    /// `value` and return a reference offering modifiable access to this
    /// object.
    PushHeader&
    setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::Enum value);

    /// Set the number of words of the header of this message to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    PushHeader& setHeaderWords(int value);

    /// Set the queue id to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PushHeader& setQueueId(int value);

    /// Set the messageGUID to the specified `value` and return a reference
    /// offering modifiable access to this object.
    PushHeader& setMessageGUID(const bmqt::MessageGUID& value);

    /// Return a reference offering modifiable access to Schema Id
    /// associated with this object.
    SchemaWireId& schemaId();

    // ACCESSORS

    /// Return the flags mask of this header.
    int flags() const;

    /// Return the number of words of this entire event.
    int messageWords() const;

    /// Return the number of words of the options area of this event.
    int optionsWords() const;

    /// Return the compression algorithm type of this event.
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType() const;

    /// Return the number of words of the header of this event.
    int headerWords() const;

    /// Return the queue id associated to this event.
    int queueId() const;

    /// Return the messageGUID associated to this event.
    const bmqt::MessageGUID& messageGUID() const;

    /// Return the binary protocol representation of Schema Id.
    const SchemaWireId& schemaId() const;
};

// ======================
// struct PushHeaderFlags
// ======================

/// This struct defines the meanings of each bits of the flags field of the
/// `PushHeader` structure.
struct PushHeaderFlags {
    // TYPES
    enum Enum {
        e_IMPLICIT_PAYLOAD   = (1 << 0),
        e_MESSAGE_PROPERTIES = (1 << 1),
        e_OUT_OF_ORDER       = (1 << 2),
        e_UNUSED4            = (1 << 3)
    };

    // PUBLIC CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that a
    /// PushHeader's `Flags` field is a supported type.
    static const int k_LOWEST_SUPPORTED_PUSH_FLAG = e_IMPLICIT_PAYLOAD;

    /// NOTE: This value must always be equal to the highest *supported*
    /// type in the enum because it is being used as an upper bound to
    /// verify a PushHeader's `Flags` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_PUSH_FLAG = e_MESSAGE_PROPERTIES;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify a
    /// PushHeader's `Flags` field is a valid type.
    static const int k_HIGHEST_PUSH_FLAG = e_UNUSED4;

    /// Used by test drivers
    static const int k_VALUE_COUNT = PushHeader::k_FLAGS_NUM_BITS;

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
                               PushHeaderFlags::Enum value,
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
    static const char* toAscii(PushHeaderFlags::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(PushHeaderFlags::Enum*   out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, PushHeaderFlags::Enum value);

// =========================
// struct PushHeaderFlagUtil
// =========================

/// This component provides utility methods for `bmqp::PushHeaderFlags`.
struct PushHeaderFlagUtil {
    // CLASS METHODS

    /// Set the specified `flag` to true in the bit-mask in the specified
    /// `flags`.
    static void setFlag(int* flags, PushHeaderFlags::Enum flag);

    /// Resets the specified `flag` to false in the bit-mask in the
    /// specified `flags`.
    static void unsetFlag(int* flags, PushHeaderFlags::Enum flag);

    /// Return true if the bit-mask in the specified `flags` has the
    /// specified `flag` set, or false if not.
    static bool isSet(int flags, PushHeaderFlags::Enum flag);

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

// ====================
// struct ConfirmHeader
// ====================

/// This struct represents header for a `CONFIRM` event.  A `CONFIRM` event
/// is the event sent by a client to the broker to signify it's done
/// processing a specific message and the broker can dispose of it.
struct ConfirmHeader {
    // ConfirmHeader structure datagram [4 bytes (followed by one or multiple
    //                                            ConfirmMessage)]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |  HW   |  PMW  |                   Reserved                    |
    //   +---------------+---------------+---------------+---------------+
    //       HW..: HeaderWords
    //       PMW.: PerMessageWords
    //       R...: Reserved
    //
    //  HeaderWords (HW)......: Total size (words) of this ConfirmHeader
    //  PerMessageWords (PMW).: Size (words) used for each ConfirmMessage in
    //                          the payload following this ConfirmHeader
    //  Reserved (R)..........: For alignment and extension ~ must be 0
    //..

  private:
    // PRIVATE CONSTANTS
    static const int k_HEADER_WORDS_NUM_BITS  = 4;
    static const int k_PER_MSG_WORDS_NUM_BITS = 4;

    static const int k_HEADER_WORDS_START_IDX  = 4;
    static const int k_PER_MSG_WORDS_START_IDX = 0;

    static const int k_HEADER_WORDS_MASK;
    static const int k_PER_MSG_WORDS_MASK;

    // DATA
    unsigned char d_headerWordsAndPerMsgWords;
    // Total size (words) of this header and number of words of
    // each ConfirmMessage in the payload that follows.

    BSLA_MAYBE_UNUSED unsigned char d_reserved[3];
    // Reserved

  public:
    // PUBLIC CLASS DATA

    /// Maximum size (bytes) of an `ConfirmHeader`.
    static const int k_MAX_HEADER_SIZE = ((1 << k_HEADER_WORDS_NUM_BITS) - 1) *
                                         Protocol::k_WORD_SIZE;

    /// Maximum size (bytes) of an `ConfirmMessage`.
    static const int k_MAX_PERMESSAGE_SIZE = ((1 << k_PER_MSG_WORDS_NUM_BITS) -
                                              1) *
                                             Protocol::k_WORD_SIZE;

    /// Minimum size (bytes) of a `ConfirmHeader` (which is sufficient to
    /// capture header words).  This value should *never* change.
    static const int k_MIN_HEADER_SIZE = 1;

  public:
    // CREATORS

    /// Create this object where all fields except `headerWords` and
    /// `perMessageWords` are set to zero.  Field `headerWords` is set to
    /// size of this object, the value for which is directly derived from
    /// `sizeof()` operator; and field `perMessageWords` is set to the size
    /// of the `ConfirmMessage` struct.
    explicit ConfirmHeader();

    // MANIPULATORS

    /// Set the number of words of this header to the specified `value` and
    /// return a reference offering modifiable access to this object.
    ConfirmHeader& setHeaderWords(int value);

    /// Set the number of words of each ConfirmMessage in the payload that
    /// follows this header to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ConfirmHeader& setPerMessageWords(int value);

    // ACCESSORS

    /// Return the number of words of this header.
    int headerWords() const;

    /// Return the number of words of each ConfirmMessage in the payload
    /// that follows.
    int perMessageWords() const;
};

// =====================
// struct ConfirmMessage
// =====================

/// This struct defines the (repeated) payload following the `ConfirmHeader`
/// struct.
struct ConfirmMessage {
    // ConfirmMessage structure datagram [24 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |                            QueueId                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                          MessageGUID                          |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                          SubQueueId                           |
    //   +---------------+---------------+---------------+---------------+
    //
    //  QueueId.......: Id of the queue (as advertised during open queue by the
    //                  consumer of this message)
    //  MessageGUID...: MessageGUID associated to this message by the broker
    //  SubQueueId....: Id of the view associated to this message and queueId
    //                  , i.e. "SubQueue" (as announced during open queue by
    //                  the consumer of this message).
    //..
    //

  private:
    // DATA
    bdlb::BigEndianInt32 d_queueId;
    // Queue Id.

    unsigned char d_messageGUID[bmqt::MessageGUID::e_SIZE_BINARY];
    // MessageGUID associated to this message.

    bdlb::BigEndianInt32 d_subQueueId;
    // SubQueue Id.

  public:
    // CREATORS

    /// Create this object where all fields are set to zero.
    explicit ConfirmMessage();

    // MANIPULATORS

    /// Set the queue id to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ConfirmMessage& setQueueId(int value);

    /// Set the messageGUID of this message to the specified `value` and
    /// return a reference offering modifiable access to this object.
    ConfirmMessage& setMessageGUID(const bmqt::MessageGUID& value);

    /// Set the subqueue id to the specified `value` and return a reference
    /// offering modifiable access to this object.
    ConfirmMessage& setSubQueueId(int value);

    // ACCESSORS

    /// Return the queueId associated to this message.
    int queueId() const;

    /// Return the messageGUID of this message.
    const bmqt::MessageGUID& messageGUID() const;

    /// Return the subQueueId associated to this message.
    int subQueueId() const;
};

// ===================
// struct RejectHeader
// ===================

/// This struct represents header for a `Reject` event.  A `Reject` event
/// is the event sent by a downstream broker to an upstream broker to
/// signify that a specific message has been rejected after a number of
/// attempts to deliver the message to consumers.
struct RejectHeader {
    // RejectHeader structure datagram [4 bytes (followed by one or multiple
    //                                           RejectMessage)]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |  HW   |  PMW  |                   Reserved                    |
    //   +---------------+---------------+---------------+---------------+
    //       HW..: HeaderWords
    //       PMW.: PerMessageWords
    //       R...: Reserved
    //
    //  HeaderWords (HW)......: Total size (words) of this RejectHeader
    //  PerMessageWords (PMW).: Size (words) used for each RejectMessage in
    //                          the payload following this RejectHeader
    //  Reserved (R)..........: For alignment and extension ~ must be 0
    //..

  private:
    // PRIVATE CONSTANTS
    static const int k_HEADER_WORDS_NUM_BITS  = 4;
    static const int k_PER_MSG_WORDS_NUM_BITS = 4;

    static const int k_HEADER_WORDS_START_IDX  = 4;
    static const int k_PER_MSG_WORDS_START_IDX = 0;

    static const int k_HEADER_WORDS_MASK;
    static const int k_PER_MSG_WORDS_MASK;

    // DATA
    unsigned char d_headerWordsAndPerMsgWords;
    // Total size (words) of this header and number of words of
    // each RejectMessage in the payload that follows.

    BSLA_MAYBE_UNUSED unsigned char d_reserved[3];
    // Reserved

  public:
    // PUBLIC CLASS DATA

    /// Maximum size (bytes) of an `RejectHeader`.
    static const int k_MAX_HEADER_SIZE = ((1 << k_HEADER_WORDS_NUM_BITS) - 1) *
                                         Protocol::k_WORD_SIZE;

    /// Maximum size (bytes) of an `RejectMessage`.
    static const int k_MAX_PERMESSAGE_SIZE = ((1 << k_PER_MSG_WORDS_NUM_BITS) -
                                              1) *
                                             Protocol::k_WORD_SIZE;

    /// Minimum size (bytes) of a `RejectHeader` (which is sufficient to
    /// capture header words).  This value should *never* change.
    static const int k_MIN_HEADER_SIZE = 1;

  public:
    // CREATORS

    /// Create this object where all fields except `headerWords` and
    /// `perMessageWords` are set to zero.  Field `headerWords` is set to
    /// size of this object, the value for which is directly derived from
    /// `sizeof()` operator; and field `perMessageWords` is set to the size
    /// of the `RejectMessage` struct.
    explicit RejectHeader();

    // MANIPULATORS

    /// Set the number of words of this header to the specified `value` and
    /// return a reference offering modifiable access to this object.
    RejectHeader& setHeaderWords(int value);

    /// Set the number of words of each RejectMessage in the payload that
    /// follows this header to the specified `value` and return a reference
    /// offering modifiable access to this object.
    RejectHeader& setPerMessageWords(int value);

    // ACCESSORS

    /// Return the number of words of this header.
    int headerWords() const;

    /// Return the number of words of each RejectMessage in the payload
    /// that follows.
    int perMessageWords() const;
};

// ====================
// struct RejectMessage
// ====================

/// This struct defines the (repeated) payload following the `RejectHeader`
/// struct.
struct RejectMessage {
    // RejectMessage structure datagram [24 bytes]:
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |                            QueueId                            |
    //   +---------------+---------------+---------------+---------------+
    //   |                          MessageGUID                          |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                      MessageGUID (cont.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                          SubQueueId                           |
    //   +---------------+---------------+---------------+---------------+
    //
    //  QueueId.......: Id of the queue (as advertised during open queue by the
    //                  consumer of this message)
    //  MessageGUID...: MessageGUID associated to this message by the broker
    //  SubQueueId....: Id of the view associated to this message and queueId
    //                  , i.e. "SubQueue" (as announced during open queue by
    //                  the consumer of this message).
    //..
    //

  private:
    // DATA
    bdlb::BigEndianInt32 d_queueId;
    // Queue Id.

    unsigned char d_messageGUID[bmqt::MessageGUID::e_SIZE_BINARY];
    // MessageGUID associated to this message.

    bdlb::BigEndianInt32 d_subQueueId;
    // SubQueue Id.

  public:
    // CREATORS

    /// Create this object where all fields are set to zero.
    explicit RejectMessage();

    // MANIPULATORS

    /// Set the queue id to the specified `value` and return a reference
    /// offering modifiable access to this object.
    RejectMessage& setQueueId(int value);

    /// Set the messageGUID of this message to the specified `value` and
    /// return a reference offering modifiable access to this object.
    RejectMessage& setMessageGUID(const bmqt::MessageGUID& value);

    /// Set the subqueue id to the specified `value` and return a reference
    /// offering modifiable access to this object.
    RejectMessage& setSubQueueId(int value);

    // ACCESSORS

    /// Return the queueId associated to this message.
    int queueId() const;

    /// Return the messageGUID of this message.
    const bmqt::MessageGUID& messageGUID() const;

    /// Return the subQueueId associated to this message.
    int subQueueId() const;
};

// =========================
// struct ReplicationReceipt
// =========================

/// This struct represents a Replication Receipt
struct ReplicationReceipt {
    bdlb::BigEndianUint32 d_partitionId;

    bdlb::BigEndianUint32 d_seqNumUpperBits;

    bdlb::BigEndianUint32 d_seqNumLowerBits;

    bdlb::BigEndianUint32 d_primaryLeaseId;

    // CREATORS
    ReplicationReceipt();

    // MANIPULATORS
    ReplicationReceipt& setPartitionId(unsigned int value);

    ReplicationReceipt& setSequenceNum(bsls::Types::Uint64 value);

    ReplicationReceipt& setPrimaryLeaseId(unsigned int value);

    // ACCESSORS
    unsigned int partitionId() const;

    bsls::Types::Uint64 sequenceNum() const;

    unsigned int primaryLeaseId() const;
};

// =========================
// struct StorageMessageType
// =========================

/// This struct defines the type of storage messages exchanged between
/// BlazingMQ brokers within a cluster.
struct StorageMessageType {
    // TYPES
    enum Enum {
        e_UNDEFINED  = 0,
        e_DATA       = 1,
        e_QLIST      = 2,
        e_CONFIRM    = 3,
        e_DELETION   = 4,
        e_JOURNAL_OP = 5,
        e_QUEUE_OP   = 6
    };

    // PUBLIC CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that an
    /// StorageMessage's `type` field is a supported type.
    static const int k_LOWEST_SUPPORTED_STORAGE_MSG_TYPE = e_DATA;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify an
    /// StorageMessage's `type` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_STORAGE_MSG_TYPE = e_QUEUE_OP;

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
    /// `StorageMessageType::Enum` value.
    static bsl::ostream& print(bsl::ostream&            stream,
                               StorageMessageType::Enum value,
                               int                      level          = 0,
                               int                      spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << StorageMessageType::toAscii(
    ///                                        StorageMessageType::e_DATA);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// DATA
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(StorageMessageType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, StorageMessageType::Enum value);

// ====================
// struct StorageHeader
// ====================

/// This struct represents the header for a `STORAGE` message.  A `STORAGE`
/// message is exchanged strictly among the nodes inside the cluster.  It is
/// never sent to clients or proxies.
struct StorageHeader {
    // StorageHeader structure datagram (12 bytes, followed by payload, if any)
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |    F    |                MessageWords                         |
    //   +---------------+---------------+---------------+---------------+
    //   | R |SPV|  HW   |  MessageType  |         PartitionId           |
    //   +---------------+---------------+---------------+---------------+
    //   |                   Journal Offset Word                         |
    //   +---------------+---------------+---------------+---------------+
    //       F...: Flags
    //       R...: Reserved
    //       SPV.: File Storage Protocol Version
    //       HW..: HeaderWords
    //
    //  Flags (F)...................: Flags.  See StorageHeaderFlags struct.
    //  MessageWords................: Total size (words) of the message,
    //                                including this header and the message
    //                                payload.
    //  Reserved (R)................: Reserved for future use.
    //  File Storage Protocol (SPV).: Protocol version of FileStore.  This
    //                                protocol version is used to indicate the
    //                                way receiver should interpret the message
    //                                payload.
    //  HeaderWords (HW)............: Total size (words) of this StorageHeader.
    //  MessageType.................: Type of storage message.  See
    //                                StorageMessageType for details.
    //  PartitionId.................: PartitionId to which this message
    //                                belongs.
    //  Journal Offset Words .......: Offset of the JOURNAL at the node which
    //                                sent this message.
    //..
    //
    // NOTE: From the first byte of the StorageHeader, the payload can be found
    //       by: ptr += 4 * headerWords
    //
    // The data payload then follows this header.  Data must be padded.  Note
    // that data payload is optional.  There is no data payload for
    // 'mqbs::StorageMessageType::e_SYNCPOINT'.

  private:
    // FRIENDS
    friend struct StorageHeaderFlags;
    // So that it can access the 'k_FLAGS_NUM_BITS' value.

    // PRIVATE CONSTANTS
    static const int k_FLAGS_NUM_BITS        = 5;
    static const int k_MSG_WORDS_NUM_BITS    = 27;
    static const int k_RESERVED_NUM_BITS     = 2;
    static const int k_SPV_NUM_BITS          = 2;
    static const int k_HEADER_WORDS_NUM_BITS = 4;

    static const int k_FLAGS_START_IDX        = 27;
    static const int k_MSG_WORDS_START_IDX    = 0;
    static const int k_RESERVED_START_IDX     = 6;
    static const int k_SPV_START_IDX          = 4;
    static const int k_HEADER_WORDS_START_IDX = 0;

    static const int k_FLAGS_MASK;
    static const int k_MSG_WORDS_MASK;
    static const int k_SPV_MASK;
    static const int k_HEADER_WORDS_MASK;

    // DATA
    bdlb::BigEndianUint32 d_flagsAndMessageWords;
    // Flags, and total size (in words) of the message
    // (including this header and message payload)

    unsigned char d_reservedAndSpvAndHeaderWords;

    unsigned char d_messageType;

    bdlb::BigEndianUint16 d_partitionId;

    bdlb::BigEndianUint32 d_journalOffsetWords;
    // Journal offset in words.

  public:
    // PUBLIC CLASS DATA

    /// Minimum size (bytes) of a `StorageHeader` (that is sufficient to
    /// capture header words).  This value should *never* change.
    static const int k_MIN_HEADER_SIZE = 5;

    /// Maximum size (bytes) of a full StorageMessage with header and
    /// payload.
    static const unsigned int k_MAX_SIZE = ((1 << k_MSG_WORDS_NUM_BITS) - 1) *
                                           Protocol::k_WORD_SIZE;

    /// Maximum size (bytes) of a StorageMessage payload (without header).
    ///
    /// TBD: The constant `StorageHeader::k_MAX_SIZE_SOFT` is used by
    /// `bmqp::StorageEventBuilder::packMessage` (the replication logic) to
    /// ensure that the size of the outgoing bmqp event is not bigger than
    /// this constant.  A PUT message is permitted to have a maximum size of
    /// 64MB (see `PutHeader::k_MAX_PAYLOAD_SIZE_SOFT`), and the replication
    /// layer must be able to build and send that message.  Replication
    /// layer adds its own header and journal record to the PUT message,
    /// such that the size of an outgoing storage message can exceed
    /// `PutHeader::k_MAX_PAYLOAD_SIZE_SOFT`.  So, we assign a value of 65MB
    /// to `StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT` such that a PUT message
    /// having the maximum allowable value is processed through the BMQ
    /// replication pipeline w/o any issues.  Also see the comments in
    /// `EventHeader::k_MAX_SIZE_SOFT` constant.
    ///
    /// NOTE: the protocol supports up to k_MAX_SIZE for the entire
    ///       StorageMessage with payload, but the StorageEventBuilder only
    ///       validates the payload size against this size; so this
    ///       k_MAX_PAYLOAD_SIZE_SOFT can be increased but not up to
    ///       `k_MAX_SIZE`.
    static const unsigned int k_MAX_PAYLOAD_SIZE_SOFT = (64 + 1) * 1024 * 1024;

    /// Maximum value of the storage protocol version
    static const int k_MAX_SPV_VALUE = (1 << k_SPV_NUM_BITS) - 1;

    /// Maximum size (bytes) of a `StorageHeader`.
    static const int k_MAX_HEADER_SIZE = ((1 << k_HEADER_WORDS_NUM_BITS) - 1) *
                                         Protocol::k_WORD_SIZE;

  public:
    // CREATORS

    /// Create this object where `headerWords` and `messageWords` fields are
    /// set to size of this object, the value for which is directly derived
    /// from `sizeof()` operator.  All other fields, *including* file
    /// storage protocol version, are set to zero.
    StorageHeader();

    // MANIPULATORS

    /// Set the flags mask to the specified `value` and return a reference
    /// offering modifiable access to this object.
    StorageHeader& setFlags(int value);

    /// Set the number of words of this entire event (including this header,
    /// the options and the message payload) to the specified `value` and
    /// return a reference offering modifiable access to this object.
    StorageHeader& setMessageWords(int value);

    /// Set the file storage protocol version of this message to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    StorageHeader& setStorageProtocolVersion(int value);

    /// Set the number of words of the header of this message to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    StorageHeader& setHeaderWords(unsigned int value);

    /// Set the type of this storage message to the specified `value` and
    /// return a reference offering modifiable access to this object.
    StorageHeader& setMessageType(StorageMessageType::Enum value);

    /// Set the partition of this storage message to the specified `value`
    /// and return a reference offering modifiable access to this object.
    StorageHeader& setPartitionId(unsigned int value);

    /// Set the JOURNAL offset words to the specified `value` and return a
    /// reference offering modifiable access to this object.
    StorageHeader& setJournalOffsetWords(unsigned int value);

    // ACCESSORS

    /// Return the flags mask of this header.
    int flags() const;

    /// Return the number of words of this entire event.
    int messageWords() const;

    /// Return the file storage protocol version associated with this
    /// object.
    int storageProtocolVersion() const;

    /// Return the number of words of the header of this event.
    int headerWords() const;

    /// Return the type of this message.
    StorageMessageType::Enum messageType() const;

    /// Return the partitionId associated with this object.
    unsigned int partitionId() const;

    /// Return the JOURNAL offset words associated with this object.
    unsigned int journalOffsetWords() const;
};

// =========================
// struct StorageHeaderFlags
// =========================

/// This struct defines the meanings of each bits of the flags field of the
/// `StorageHeader` structure.
struct StorageHeaderFlags {
    // TYPES
    enum Enum {
        e_RECEIPT_REQUESTED = (1 << 0)  // Ack for STORAGE msg is requested
        ,
        e_UNUSED2 = (1 << 1),
        e_UNUSED3 = (1 << 2),
        e_UNUSED4 = (1 << 3)
    };

    // PUBLIC CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that a
    /// StorageHeader's `Flags` field is a supported type.
    static const int k_LOWEST_SUPPORTED_STORAGE_FLAG = e_RECEIPT_REQUESTED;

    /// NOTE: This value must always be equal to the highest *supported*
    /// type in the enum because it is being used as an upper bound to
    /// verify a StorageHeader's `Flags` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_STORAGE_FLAG = e_RECEIPT_REQUESTED;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify a
    /// StorageHeader's `Flags` field is a valid type.
    static const int k_HIGHEST_STORAGE_FLAG = e_UNUSED4;

    /// Used by test drivers
    static const int k_VALUE_COUNT = StorageHeader::k_FLAGS_NUM_BITS;

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
    static bsl::ostream& print(bsl::ostream&            stream,
                               StorageHeaderFlags::Enum value,
                               int                      level          = 0,
                               int                      spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(StorageHeaderFlags::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(StorageHeaderFlags::Enum* out,
                          const bslstl::StringRef&  str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, StorageHeaderFlags::Enum value);

// ============================
// struct StorageHeaderFlagUtil
// ============================

/// This component provides utility methods for `bmqp::StorageHeaderFlags`.
struct StorageHeaderFlagUtil {
    // CLASS METHODS

    /// Set the specified `flag` to true in the bit-mask in the specified
    /// `flags`.
    static void setFlag(int* flags, StorageHeaderFlags::Enum flag);

    /// Resets the specified `flag` to false in the bit-mask in the
    /// specified `flags`.
    static void unsetFlag(int* flags, StorageHeaderFlags::Enum flag);

    /// Return true if the bit-mask in the specified `flags` has the
    /// specified `flag` set, or false if not.
    static bool isSet(unsigned char flags, StorageHeaderFlags::Enum flag);

    /// Check whether the specified `flags` represent a valid combination of
    /// flags to use for opening a queue.  Return true if it does, or false
    /// if some exclusive flags are both set, and populate the specified
    /// `errorDescription` with the reason of the failure.
    static bool isValid(bsl::ostream& errorDescription, unsigned char flags);

    /// Print the ascii-representation of all the values set in the
    /// specified `flags` to the specified `stream`.  Each value is `,`
    /// separated.
    static bsl::ostream& prettyPrint(bsl::ostream& stream,
                                     unsigned char flags);

    /// Convert the string representation of the enum bit mask from the
    /// specified `str` (which format corresponds to the one of the
    /// `prettyPrint` method) and populate the specified `out` with the
    /// result on success returning 0, or return a non-zero error code on
    /// error, populating the specified `errorDescription` with a
    /// description of the error.
    static int fromString(bsl::ostream&      errorDescription,
                          unsigned char*     out,
                          const bsl::string& str);
};

// ============================
// struct RecoveryFileChunkType
// ============================

/// This struct defines various types that a file chunk can belong to during
/// recovery phase in BMQ.
struct RecoveryFileChunkType {
    // TYPES
    enum Enum { e_UNDEFINED = 0, e_DATA = 1, e_JOURNAL = 2, e_QLIST = 3 };

    // PUBLIC CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that an
    /// RecoveryFileChunk's `type` field is a supported type.
    static const int k_LOWEST_SUPPORTED_RECOVERY_CHUNK_TYPE = e_DATA;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify an
    /// RecoveryFileChunk's `type` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_RECOVERY_CHUNK_TYPE = e_QLIST;

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
    /// `RecoveryFileChunkType::Enum` value.
    static bsl::ostream& print(bsl::ostream&               stream,
                               RecoveryFileChunkType::Enum value,
                               int                         level          = 0,
                               int                         spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << RecoveryFileChunkType::toAscii(
    ///                                  RecoveryFileChunkType::e_JOURNAL);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// JOURNAL
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(RecoveryFileChunkType::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&               stream,
                         RecoveryFileChunkType::Enum value);

// =====================
// struct RecoveryHeader
// =====================

/// This struct represents the header for a `RECOVERY` message.  A
/// `RECOVERY` message is exchanged strictly among the nodes inside the
/// cluster.  It is never sent to clients or proxies.
struct RecoveryHeader {
    // RecoveryHeader structure datagram (28 bytes, followed by payload)
    //..
    //   +---------------+---------------+---------------+---------------+
    //   |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    //   +---------------+---------------+---------------+---------------+
    //   |F|  R  |                 MessageWords                          |
    //   +---------------+---------------+---------------+---------------+
    //   |       R       |   HW  |  FCT  |         PartitionId           |
    //   +---------------+---------------+---------------+---------------+
    //   |                     Chunk Sequence Number                     |
    //   +---------------+---------------+---------------+---------------+
    //   |                           Chunk MD5                           |
    //   +---------------+---------------+---------------+---------------+
    //   |                       Chunk MD5 (contd.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                       Chunk MD5 (contd.)                      |
    //   +---------------+---------------+---------------+---------------+
    //   |                       Chunk MD5 (contd.)                      |
    //   +---------------+---------------+---------------+---------------+
    //
    //       F....: Final Chunk Bit
    //       R....: Reserved
    //       HW...: HeaderWords
    //       FCT..: File Chunk Type
    //
    //  Final Chunk Bit (F)....: If set, this bit indicates that this recovery
    //                           message is final.
    //  MessageWords...........: Total size (words) of the message, including
    //                           this header and the message payload.
    //  Reserved (R)...........: Reserved for future use ~ must be 0
    //  HeaderWords (HW).......: Total size (words) of this StorageHeader.
    //  File Chunk Type (FCT)..: File type to which this chunk belongs.
    //  PartitionId............: PartitionId to which this message belongs.
    //  Chunk Sequence Number..: Sequence number of the chunk.
    //  Chunk MD5 Digest.......: MD5 digest of the chunk.
    //..
    //
    // NOTE: From the first byte of the 'RecoveryHeader', the payload can be
    //       found by: ptr += 4 * headerWords
    //
    // The data payload then follows this header.  Data must be padded.

  public:
    // PUBLIC CLASS DATA

    /// Length of MD5 digest.
    static const int k_MD5_DIGEST_LEN = 16;

  private:
    // PRIVATE CONSTANTS
    static const int k_FCB_NUM_BITS             = 1;  // FCB == Final Chunk Bit
    static const int k_MSG_WORDS_NUM_BITS       = 28;
    static const int k_HEADER_WORDS_NUM_BITS    = 4;
    static const int k_FILE_CHUNK_TYPE_NUM_BITS = 4;

    static const int k_FCB_START_IDX             = 31;
    static const int k_MSG_WORDS_START_IDX       = 0;
    static const int k_HEADER_WORDS_START_IDX    = 4;
    static const int k_FILE_CHUNK_TYPE_START_IDX = 0;

    static const int k_FCB_MASK;
    static const int k_MSG_WORDS_MASK;
    static const int k_HEADER_WORDS_MASK;
    static const int k_FILE_CHUNK_TYPE_MASK;

    // DATA
    bdlb::BigEndianUint32 d_fcbAndReservedAndMessageWords;
    // Final Chunk Bit (FCB) , reserved
    // bits and total size (in words) of
    // the message (including this header
    // and message payload)

    BSLA_MAYBE_UNUSED char d_reserved;

    unsigned char d_headerWordsAndFileChunkType;

    bdlb::BigEndianUint16 d_partitionId;

    bdlb::BigEndianUint32 d_chunkSequenceNumber;

    char d_md5Digest[k_MD5_DIGEST_LEN];

  public:
    // PUBLIC CLASS DATA

    /// Minimum size (bytes) of a `RecoveryHeader` (that is sufficient to
    /// capture header words).  This value should *never* change.
    static const int k_MIN_HEADER_SIZE = 6;

    /// Maximum size (bytes) of a full RecoveryMessage with header and
    /// payload.
    static const unsigned int k_MAX_SIZE = ((1U << k_MSG_WORDS_NUM_BITS) - 1) *
                                           Protocol::k_WORD_SIZE;

    /// Maximum size (bytes) of a RecoveryMessage payload (ie, a chunk)
    /// without header.
    ///
    /// NOTE: the protocol supports up to k_MAX_SIZE for the entire
    ///       RecoveryMessage with payload, but RecoveryEventBuilder only
    ///       validates the payload size against this size; so this
    ///       k_MAX_PAYLOAD_SIZE_SOFT can be increased but not up to
    ///       `k_MAX_SIZE`.
    static const unsigned int k_MAX_PAYLOAD_SIZE_SOFT = 64 * 1024 * 1024;

    /// Maximum value of file chunk type.
    static const int k_MAX_FILE_CHUNK_TYPE_VALUE =
        (1 << k_FILE_CHUNK_TYPE_NUM_BITS) - 1;

    /// Maximum size (bytes) of a `RecoveryHeader`.
    static const int k_MAX_HEADER_SIZE = ((1 << k_HEADER_WORDS_NUM_BITS) - 1) *
                                         Protocol::k_WORD_SIZE;

  public:
    // CREATORS

    /// Create this object where `headerWords` and `messageWords` fields are
    /// set to size of this object, the value for which is directly derived
    /// from `sizeof()` operator.  All other fields are set to zero.
    RecoveryHeader();

    // MANIPULATORS

    /// Set the bit indicating that the chunk is the final one in the
    /// sequence, and return a reference offering modifiable access to this
    /// object.
    RecoveryHeader& setFinalChunkBit();

    /// Set the number of words of this entire event (including this header,
    /// the options and the message payload) to the specified `value` and
    /// return a reference offering modifiable access to this object.
    RecoveryHeader& setMessageWords(int value);

    /// Set the number of words of the header of this message to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    RecoveryHeader& setHeaderWords(int value);

    /// Set the file type of the chunk to the specified `value` and return a
    /// reference offering modifiable access to this object.
    RecoveryHeader& setFileChunkType(RecoveryFileChunkType::Enum value);

    /// Set the partitionId of the chunk to the specified `value`, and
    /// return a reference offering modifiable access to this object.
    RecoveryHeader& setPartitionId(unsigned int value);

    /// Set the sequence number of the chunk to the specified `value`, and
    /// return a reference offering modifiable access to this object.
    RecoveryHeader& setChunkSequenceNumber(unsigned int value);

    /// Set the MD5 digest of the chunk to the specified `value`, and return
    /// a reference offering modifiable access to this object.
    RecoveryHeader& setMd5Digest(const char* value);

    // ACCESSORS
    bool isFinalChunk() const;

    /// Return the number of words of this entire event.
    int messageWords() const;

    /// Return the number of words in this header.
    int headerWords() const;

    /// Return the file type of the chunk.
    RecoveryFileChunkType::Enum fileChunkType() const;

    /// Return the partitionId of the chunk.
    unsigned int partitionId() const;

    /// Return the sequence number of the chunk.
    unsigned int chunkSequenceNumber() const;

    /// Return the MD5 digest of the chunk.
    const char* md5Digest() const;
};

/// This VST controls MessageProperties logic and storage.
/// The intended use is for:
///      mqbi::DispatcherEvent,
///      bmqp::PutHeader,
///      bmqp::PushHeder,
///      mqbi::StorageMessageAttributes.
///      mqbs::DataHeader,
///      mqbs::DataStoreRecord.
///
/// All those components need to remember whether MessageProperties is
/// present and if so, whether the encoding is extended and what is the
/// schema id and whether the schema id is recycled.
/// This used to be just one `bool`, now we need `bool` and `unsigned`.
/// In addition, this component can load binary protocol representation into
/// an instance of template parameter `HEADER` which should expose
/// `setSchemaId` setter, `Flags` and `FlagUtil` types, and
/// `Flags::e_MESSAGE_PROPERTIES` constant.
///
/// `makeNoSchema` means the old style and the default
/// `MessagePropertiesInfo` ctor means no `MessageProperties`.
///
/// `makeInvalidSchema` means the new style <u>without</u> schema.
///
/// The `isRecycled` argument for the `MessagePropertiesInfo` ctor is what
/// makes `makeInvalidSchema` and `makeNoSchema` different. The id is the
/// same in both cases - k_NO_SCHEMA.  The recycled bit makes the wired
/// representation of no-schema-new-style different from the
/// no-schema-old-style.
///
///                          Flags::e_MESSAGE_PROPERTIES
///                  |   present                         |   not present
///  d_schemaWireId  |                                   |
///  ------------------------------------------------------------------
///               0  |   old style                       |   no MPS
///               1  |   new style, w/o schema           |   ASSERT_SAFE
///        even > 0  |   new style, w/ schema            |   ASSERT_SAFE
///         odd > 1  |   new style, w/ schema, recycled  |   ASSERT_SAFE
struct MessagePropertiesInfo {
  public:
    // PUBLIC TYPES
    typedef unsigned short SchemaIdType;

    /// Use 1 bit (least significant) as a flag to indicate recycled id.
    static const SchemaIdType k_MAX_SCHEMA = SchemaWireId::k_MAX_VALUE >> 1;

    static const SchemaIdType k_NO_SCHEMA = 0;

  private:
    // PRIVATE TYPES
    static const SchemaIdType k_RECYCLED_SCHEMA = 1;
    static const SchemaIdType k_NO_WIRE_SCHEMA  = 0;

  private:
    // PRIVATE DATA
    bool     d_isPresent;
    unsigned d_schemaWireId;

  private:
    // PRIVATE MANIPULATORS

    /// Make this object to represent the specified
    void make(SchemaIdType schemaId, bool isRecycled);

  public:
    // CREATORS

    /// Construct object indicating no MessageProperties presence.
    MessagePropertiesInfo();

    /// Construct object indicating MessageProperties presence as encoded in
    /// the specified `header` and load Schema id from the `header` into
    /// this object.
    template <class HEADER>
    MessagePropertiesInfo(const HEADER& header);

    /// Copy construct bit copy of the specified `other`.
    MessagePropertiesInfo(const MessagePropertiesInfo& other);

    /// Construct object indicating MessageProperties presence as the
    /// specified `isPresent` with Schema id as the specified `schemaId`
    /// and the specified `isRecycled`.
    MessagePropertiesInfo(bool         isPresent,
                          SchemaIdType schemaId,
                          bool         isRecycled);

    MessagePropertiesInfo& operator=(const MessagePropertiesInfo& rvalue);

    // ACCESSORS

    /// Set or unset `e_MESSAGE_PROPERTIES` flag in the specified `header`
    /// and load the binary protocol representation of Schema id according
    /// to the state of this object.
    template <class HEADER>
    void applyTo(HEADER* header) const;

    /// Return Schema id (logical, not binary protocol).
    SchemaIdType schemaId() const;

    /// Return `true` if MessageProperties are present.
    bool isPresent() const;

    /// Return `true` if Schema id is recycled.
    bool isRecycled() const;

    /// Return `true` if MessageProperties are encoded using new format -
    /// never compressed and with properties offsets instead of lengths.
    bool isExtended() const;

    /// Return `true` if this object is equivalent to the specified `other`.
    bool operator==(const MessagePropertiesInfo& other) const;

  public:
    // PUBLIC CLASS METHODS

    /// Return `true` if the specified `wire` contains Schema id.
    static bool hasSchema(const SchemaWireId& wire);

    /// Return `true` if the specified `header` contains Schema id.
    template <class HEADER>
    static bool hasSchema(const HEADER& header);

    /// Return object indicating MessageProperties presence and the new
    /// format - never compressed and with properties offsets instead of
    /// lengths.
    static const MessagePropertiesInfo makeInvalidSchema();

    /// Return object indicating MessageProperties presence and the old
    /// format - possibly compressed and with properties lengths instead of
    /// offsets.
    static const MessagePropertiesInfo makeNoSchema();
};

inline MessagePropertiesInfo::MessagePropertiesInfo()
: d_isPresent(false)
, d_schemaWireId(k_NO_WIRE_SCHEMA)
{
    // NOTHING
}

template <class HEADER>
inline MessagePropertiesInfo::MessagePropertiesInfo(const HEADER& header)
: d_isPresent(HEADER::FlagUtil::isSet(header.flags(),
                                      HEADER::Flags::e_MESSAGE_PROPERTIES))
, d_schemaWireId(k_NO_WIRE_SCHEMA)
{
    const int headerSize = header.headerWords() * Protocol::k_WORD_SIZE;

    if (headerSize >= HEADER::k_MIN_HEADER_SIZE_FOR_SCHEMA_ID) {
        d_schemaWireId = header.schemaId().value();
    }
    BSLS_ASSERT_SAFE(d_isPresent || d_schemaWireId == k_NO_WIRE_SCHEMA);
}

inline MessagePropertiesInfo::MessagePropertiesInfo(
    const MessagePropertiesInfo& other)
: d_isPresent(other.d_isPresent)
, d_schemaWireId(other.d_schemaWireId)
{
    // NOTHING
}

inline MessagePropertiesInfo::MessagePropertiesInfo(bool         isPresent,
                                                    SchemaIdType schemaId,
                                                    bool         isRecycled)
: d_isPresent(isPresent)
, d_schemaWireId(schemaId << 1)
{
    BSLS_ASSERT_SAFE(schemaId >= 0 && schemaId <= k_MAX_SCHEMA);
    if (isRecycled) {
        d_schemaWireId |= k_RECYCLED_SCHEMA;
    }
    BSLS_ASSERT_SAFE(d_isPresent || d_schemaWireId == k_NO_WIRE_SCHEMA);
}

inline MessagePropertiesInfo&
MessagePropertiesInfo::operator=(const MessagePropertiesInfo& rvalue)
{
    d_isPresent    = rvalue.d_isPresent;
    d_schemaWireId = rvalue.d_schemaWireId;

    return *this;
}

template <class HEADER>
inline void MessagePropertiesInfo::applyTo(HEADER* header) const
{
    int flags = header->flags();
    if (d_isPresent) {
        HEADER::FlagUtil::setFlag(&flags, HEADER::Flags::e_MESSAGE_PROPERTIES);
    }
    else {
        HEADER::FlagUtil::unsetFlag(&flags,
                                    HEADER::Flags::e_MESSAGE_PROPERTIES);
    }
    header->setFlags(flags);
    header->schemaId().set(d_schemaWireId);
}

// ACCESSORS
inline bool MessagePropertiesInfo::isPresent() const
{
    return d_isPresent;
}

inline bool MessagePropertiesInfo::isRecycled() const
{
    return d_schemaWireId & k_RECYCLED_SCHEMA;
}

inline bool MessagePropertiesInfo::isExtended() const
{
    return d_schemaWireId != k_NO_WIRE_SCHEMA;
}

inline MessagePropertiesInfo::SchemaIdType
MessagePropertiesInfo::schemaId() const
{
    return static_cast<SchemaIdType>(d_schemaWireId >> 1);
}

inline bool
MessagePropertiesInfo::operator==(const MessagePropertiesInfo& other) const
{
    return d_isPresent == other.d_isPresent &&
           d_schemaWireId == other.d_schemaWireId;
}

// CLASS METHODS
inline bool MessagePropertiesInfo::hasSchema(const SchemaWireId& wire)
{
    return wire.value() != k_NO_WIRE_SCHEMA;
}

template <class HEADER>
inline bool MessagePropertiesInfo::hasSchema(const HEADER& header)
{
    return hasSchema(header.schemaId());
}

inline const MessagePropertiesInfo MessagePropertiesInfo::makeInvalidSchema()
{
    return MessagePropertiesInfo(true, k_NO_SCHEMA, true);
}

inline const MessagePropertiesInfo MessagePropertiesInfo::makeNoSchema()
{
    return MessagePropertiesInfo(true, k_NO_SCHEMA, false);
}

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ==============
// struct RdaInfo
// ==============

// CREATORS
inline RdaInfo::RdaInfo()
{
    setUnlimited();
}

inline RdaInfo::RdaInfo(unsigned int internalRepresentation)
{
    BSLS_ASSERT_SAFE(internalRepresentation <= k_MAX_INTERNAL_COUNTER_VALUE);

    d_counter = static_cast<unsigned char>(internalRepresentation);
}

inline RdaInfo::RdaInfo(const RdaInfo& original)
{
    d_counter = original.d_counter;
}

// MANIPULATORS
inline RdaInfo& RdaInfo::setUnlimited()
{
    d_counter = e_UNLIMITED;

    return *this;
}

inline RdaInfo& RdaInfo::setPotentiallyPoisonous(bool flag)
{
    BSLS_ASSERT_SAFE(!isUnlimited());

    if (flag) {
        d_counter |= e_POISONOUS;
    }
    else {
        d_counter &= ~e_POISONOUS;
    }

    return *this;
}

inline RdaInfo& RdaInfo::setCounter(unsigned int counter)
{
    BSLS_ASSERT_SAFE(counter <= k_MAX_COUNTER_VALUE);
    // Drop e_UNLIMITED bit flag if set, but save e_POISONOUS bit
    d_counter = static_cast<unsigned char>(counter |
                                           (d_counter & e_POISONOUS));
    return *this;
}

// ACCESSORS
inline bool RdaInfo::isUnlimited() const
{
    return d_counter & e_UNLIMITED;
}

inline bool RdaInfo::isPotentiallyPoisonous() const
{
    return isUnlimited() ? false : d_counter & e_POISONOUS;
}

inline unsigned int RdaInfo::counter() const
{
    return isUnlimited() ? k_MAX_COUNTER_VALUE + 1
                         : d_counter & k_MAX_COUNTER_VALUE;
}

inline unsigned int RdaInfo::internalRepresentation() const
{
    return d_counter;
}

// -------------------
// struct SubQueueInfo
// -------------------

// MANIPULATORS
inline SubQueueInfo& SubQueueInfo::setId(unsigned int value)
{
    d_subQueueId = value;
    return *this;
}

// ACCESSORS
inline unsigned int SubQueueInfo::id() const
{
    return d_subQueueId;
}

inline RdaInfo& SubQueueInfo::rdaInfo()
{
    return d_rdaInfo;
}

inline const RdaInfo& SubQueueInfo::rdaInfo() const
{
    return d_rdaInfo;
}

// ---------------
// struct Protocol
// ---------------

inline bsls::Types::Uint64 Protocol::combine(unsigned int upper,
                                             unsigned int lower)
{
    bsls::Types::Uint64 value = upper;
    return (value << 32) | lower;
}

inline unsigned int Protocol::getUpper(bsls::Types::Uint64 value)
{
    return static_cast<unsigned int>((value >> 32) & 0xFFFFFFFF);
}

inline unsigned int Protocol::getLower(bsls::Types::Uint64 value)
{
    return value & 0xFFFFFFFF;
}

inline unsigned short Protocol::get48to32Bits(bsls::Types::Uint64 value)
{
    return static_cast<unsigned short>((value >> 32) & 0xFFFF);
}

inline void Protocol::split(unsigned int*       upper,
                            unsigned int*       lower,
                            bsls::Types::Uint64 value)
{
    *upper = getUpper(value);
    *lower = getLower(value);
}

inline void Protocol::split(bdlb::BigEndianUint32* upper,
                            bdlb::BigEndianUint32* lower,
                            bsls::Types::Uint64    value)
{
    *upper = getUpper(value);
    *lower = getLower(value);
}

inline void Protocol::split(bdlb::BigEndianUint16* upper,
                            bdlb::BigEndianUint32* lower,
                            bsls::Types::Uint64    value)
{
    *upper = get48to32Bits(value);
    *lower = getLower(value);
}

// ------------------
// struct EventHeader
// ------------------

// CREATORS
inline EventHeader::EventHeader()
{
    bsl::memset(this, 0, sizeof(EventHeader));
    setProtocolVersion(Protocol::k_VERSION);
    setType(EventType::e_UNDEFINED);
    setHeaderWords(sizeof(EventHeader) / Protocol::k_WORD_SIZE);
    setLength(sizeof(EventHeader));
}

inline EventHeader::EventHeader(EventType::Enum type)
{
    bsl::memset(this, 0, sizeof(EventHeader));
    setProtocolVersion(Protocol::k_VERSION);
    setType(type);
    setHeaderWords(sizeof(EventHeader) / Protocol::k_WORD_SIZE);
    setLength(sizeof(EventHeader));
}

// MANIPULATORS
inline EventHeader& EventHeader::setLength(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && static_cast<unsigned int>(value) <=
                                       (2U << k_LENGTH_NUM_BITS) - 1);

    d_fragmentBitAndLength = (d_fragmentBitAndLength & k_FRAGMENT_MASK) |
                             (value & k_LENGTH_MASK);

    return *this;
}

inline EventHeader& EventHeader::setProtocolVersion(unsigned char value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value <= (1 << k_PROTOCOL_VERSION_NUM_BITS) - 1);

    d_protocolVersionAndType = static_cast<unsigned char>(
        (d_protocolVersionAndType & k_TYPE_MASK) |
        (value << k_PROTOCOL_VERSION_START_IDX));

    return *this;
}

inline EventHeader& EventHeader::setType(EventType::Enum value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     static_cast<int>(value) <= (1 << k_TYPE_NUM_BITS) - 1);

    d_protocolVersionAndType = static_cast<unsigned char>(
        (d_protocolVersionAndType & k_PROTOCOL_VERSION_MASK) |
        (value & k_TYPE_MASK));

    return *this;
}

inline EventHeader& EventHeader::setHeaderWords(unsigned char value)
{
    d_headerWords = value;

    return *this;
}

inline EventHeader& EventHeader::setTypeSpecific(unsigned char value)
{
    d_typeSpecific = value;

    return *this;
}

// ACCESSORS
inline int EventHeader::fragmentBit() const
{
    return (d_fragmentBitAndLength & k_FRAGMENT_MASK) >> k_FRAGMENT_START_IDX;
}

inline int EventHeader::length() const
{
    return d_fragmentBitAndLength & k_LENGTH_MASK;
}

inline unsigned char EventHeader::protocolVersion() const
{
    return static_cast<unsigned char>(
        (d_protocolVersionAndType & k_PROTOCOL_VERSION_MASK) >>
        k_PROTOCOL_VERSION_START_IDX);
}

inline EventType::Enum EventHeader::type() const
{
    return static_cast<EventType::Enum>(d_protocolVersionAndType &
                                        k_TYPE_MASK);
}

inline unsigned char EventHeader::headerWords() const
{
    return d_headerWords;
}

inline unsigned char EventHeader::typeSpecific() const
{
    return d_typeSpecific;
}

// ----------------------
// struct EventHeaderUtil
// ----------------------

inline void
EventHeaderUtil::setControlEventEncodingType(EventHeader*       eventHeader,
                                             EncodingType::Enum type)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(eventHeader->type() == EventType::e_CONTROL);
    BSLS_ASSERT_SAFE(type != EncodingType::e_UNKNOWN);

    unsigned char typeSpecific = eventHeader->typeSpecific();

    // Reset the bits for encoding type
    typeSpecific &= static_cast<unsigned char>(~k_CONTROL_EVENT_ENCODING_MASK);

    // Set those bits to represent 'type'
    typeSpecific |= static_cast<unsigned char>(
        type << k_CONTROL_EVENT_ENCODING_START_IDX);

    eventHeader->setTypeSpecific(typeSpecific);
}

inline EncodingType::Enum
EventHeaderUtil::controlEventEncodingType(const EventHeader& eventHeader)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(eventHeader.type() == EventType::e_CONTROL);

    const unsigned char typeSpecific = eventHeader.typeSpecific();
    const int encodingType = (typeSpecific & k_CONTROL_EVENT_ENCODING_MASK) >>
                             k_CONTROL_EVENT_ENCODING_START_IDX;
    return static_cast<EncodingType::Enum>(encodingType);
}

// -------------------
// struct OptionHeader
// -------------------

// CREATORS
inline OptionHeader::OptionHeader()
{
    bsl::memset(this, 0, sizeof(OptionHeader));
    setType(OptionType::e_UNDEFINED);
    setWords(sizeof(OptionHeader) / Protocol::k_WORD_SIZE);
}

inline OptionHeader::OptionHeader(OptionType::Enum type, bool isPacked)
{
    bsl::memset(this, 0, sizeof(OptionHeader));
    setType(type);
    setPacked(isPacked);
    if (!isPacked) {
        setWords(sizeof(OptionHeader) / Protocol::k_WORD_SIZE);
    }
}

// MANIPULATORS
inline OptionHeader& OptionHeader::setType(OptionType::Enum value)
{
    d_content = (d_content & ~k_TYPE_MASK) | (value << k_TYPE_START_IDX);
    return *this;
}

inline OptionHeader& OptionHeader::setPacked(bool value)
{
    d_content = (d_content & ~k_PACKED_MASK) | (value << k_PACKED_START_IDX);
    return *this;
}

inline OptionHeader& OptionHeader::setTypeSpecific(unsigned char value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_TYPE_SPECIFIC_NUM_BITS) - 1);

    d_content = (d_content & ~k_TYPE_SPECIFIC_MASK) |
                (value << k_TYPE_SPECIFIC_START_IDX);
    return *this;
}

inline OptionHeader& OptionHeader::setWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_WORDS_NUM_BITS) - 1);

    d_content = (d_content & ~k_WORDS_MASK) | (value << k_WORDS_START_IDX);
    return *this;
}

// ACCESSORS
inline OptionType::Enum OptionHeader::type() const
{
    return static_cast<OptionType::Enum>((d_content & k_TYPE_MASK) >>
                                         k_TYPE_START_IDX);
}

inline bool OptionHeader::packed() const
{
    return static_cast<bool>((d_content & k_PACKED_MASK) >>
                             k_PACKED_START_IDX);
}

inline unsigned char OptionHeader::typeSpecific() const
{
    return static_cast<unsigned char>((d_content & k_TYPE_SPECIFIC_MASK) >>
                                      k_TYPE_SPECIFIC_START_IDX);
}

inline int OptionHeader::words() const
{
    return (d_content & k_WORDS_MASK) >> k_WORDS_START_IDX;
}

// -----------------------------
// class MessagePropertiesHeader
// -----------------------------

// CREATORS
inline MessagePropertiesHeader::MessagePropertiesHeader()
{
    bsl::memset(this, 0, sizeof(MessagePropertiesHeader));
    size_t headerSize = sizeof(MessagePropertiesHeader);
    setHeaderSize(static_cast<int>(headerSize));

    size_t roundedSize = headerSize + (headerSize % Protocol::k_WORD_SIZE);
    setMessagePropertiesAreaWords(
        static_cast<int>(roundedSize / Protocol::k_WORD_SIZE));

    setMessagePropertyHeaderSize(sizeof(MessagePropertyHeader));
}

// MANIPULATORS
inline MessagePropertiesHeader&
MessagePropertiesHeader::setHeaderSize(int value)
{
    value /= 2;
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= ((1 << k_HEADER_SIZE_2X_NUM_BITS) - 1));

    d_mphSize2xAndHeaderSize2x = static_cast<char>(
        (d_mphSize2xAndHeaderSize2x & k_MPH_SIZE_2X_MASK) |
        (value & k_HEADER_SIZE_2X_MASK));
    return *this;
}

inline MessagePropertiesHeader&
MessagePropertiesHeader::setMessagePropertyHeaderSize(int value)
{
    value /= 2;
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= ((1 << k_MPH_SIZE_2X_NUM_BITS) - 1));

    d_mphSize2xAndHeaderSize2x = static_cast<char>(
        (d_mphSize2xAndHeaderSize2x & k_HEADER_SIZE_2X_MASK) |
        (value << k_MPH_SIZE_2X_START_IDX));
    return *this;
}

inline MessagePropertiesHeader&
MessagePropertiesHeader::setMessagePropertiesAreaWords(int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= ((1 << k_MSG_PROPS_AREA_WORDS_NUM_BITS) - 1));

    d_msgPropsAreaWordsLower = static_cast<unsigned short>(value & 0xFFFF);
    d_msgPropsAreaWordsUpper = static_cast<unsigned short>(
        (value >> k_MSG_PROPS_AREA_WORDS_LOWER_NUM_BITS) & 0xFF);

    return *this;
}

inline MessagePropertiesHeader&
MessagePropertiesHeader::setNumProperties(int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0 && value <= k_MAX_NUM_PROPERTIES);

    d_numProperties = static_cast<unsigned char>(value);
    return *this;
}

// ACCESSORS
inline int MessagePropertiesHeader::headerSize() const
{
    return 2 * (d_mphSize2xAndHeaderSize2x & k_HEADER_SIZE_2X_MASK);
}

inline int MessagePropertiesHeader::messagePropertyHeaderSize() const
{
    return 2 * ((d_mphSize2xAndHeaderSize2x & k_MPH_SIZE_2X_MASK) >>
                k_MPH_SIZE_2X_START_IDX);
}

inline int MessagePropertiesHeader::messagePropertiesAreaWords() const
{
    int result = static_cast<unsigned short>(d_msgPropsAreaWordsLower);

    result |= (static_cast<int>(d_msgPropsAreaWordsUpper)
               << k_MSG_PROPS_AREA_WORDS_LOWER_NUM_BITS);

    return result;
}

inline int MessagePropertiesHeader::numProperties() const
{
    return d_numProperties;
}

// ---------------------------
// class MessagePropertyHeader
// ---------------------------

// CREATORS
inline MessagePropertyHeader::MessagePropertyHeader()
{
    bsl::memset(this, 0, sizeof(MessagePropertyHeader));
}

// MANIPULATORS
inline MessagePropertyHeader& MessagePropertyHeader::setPropertyType(int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0 && value <= ((1 << k_PROP_TYPE_NUM_BITS) - 1));

    d_propTypeAndPropValueLenUpper = static_cast<unsigned short>(
        (d_propTypeAndPropValueLenUpper & k_PROP_VALUE_LEN_UPPER_MASK) |
        (value << k_PROP_TYPE_START_IDX));

    return *this;
}

inline MessagePropertyHeader&
MessagePropertyHeader::setPropertyValueLength(int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0 && value <= k_MAX_PROPERTY_VALUE_LENGTH);

    d_propValueLenLower = static_cast<unsigned short>(value & 0xFFFF);

    d_propTypeAndPropValueLenUpper = static_cast<unsigned short>(
        (d_propTypeAndPropValueLenUpper & k_PROP_TYPE_MASK) |
        ((value >> k_PROP_VALUE_LEN_LOWER_NUM_BITS) &
         k_PROP_VALUE_LEN_UPPER_MASK));

    return *this;
}

inline MessagePropertyHeader&
MessagePropertyHeader::setPropertyNameLength(int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0 && value <= k_MAX_PROPERTY_NAME_LENGTH);

    d_reservedAndPropNameLen = static_cast<unsigned short>(
        value & k_PROP_NAME_LEN_MASK);

    return *this;
}

// ACCESSORS
inline int MessagePropertyHeader::propertyType() const
{
    return (d_propTypeAndPropValueLenUpper & k_PROP_TYPE_MASK) >>
           k_PROP_TYPE_START_IDX;
}

inline int MessagePropertyHeader::propertyValueLength() const
{
    int result = (d_propTypeAndPropValueLenUpper & k_PROP_VALUE_LEN_UPPER_MASK)
                 << k_PROP_VALUE_LEN_LOWER_NUM_BITS;

    result |= static_cast<unsigned short>(d_propValueLenLower);

    return result;
}

inline int MessagePropertyHeader::propertyNameLength() const
{
    return d_reservedAndPropNameLen & k_PROP_NAME_LEN_MASK;
}

// ------------------------
// struct PutHeaderFlagUtil
// ------------------------

inline void PutHeaderFlagUtil::setFlag(int* flags, PutHeaderFlags::Enum flag)
{
    *flags = (*flags | flag);
}

inline void PutHeaderFlagUtil::unsetFlag(int* flags, PutHeaderFlags::Enum flag)
{
    *flags &= ~flag;
}

inline bool PutHeaderFlagUtil::isSet(int flags, PutHeaderFlags::Enum flag)
{
    return (0 != (flags & flag));
}

// ---------------
// class SchemaWireId
// ---------------
// CREATORS
inline SchemaWireId::SchemaWireId()
: d_value(bdlb::BigEndianUint16::make(MessagePropertiesInfo::k_NO_SCHEMA))
{
    // NOTHING
}

// PUBLIC MODIFIERS
inline void SchemaWireId::set(unsigned value)
{
    // Include precondition check or adjust function signature?
    d_value = static_cast<unsigned short>(value);
}

inline unsigned SchemaWireId::value() const
{
    return static_cast<unsigned short>(d_value);
}

// ---------------
// class PutHeader
// ---------------

// CREATORS
inline PutHeader::PutHeader()
: d_flagsAndMessageWords(bdlb::BigEndianUint32::make(0))
, d_optionsAndHeaderWordsAndCAT(bdlb::BigEndianUint32::make(0))
, d_queueId(bdlb::BigEndianInt32::make(0))
, d_guidOrCorrId()
, d_crc32c(bdlb::BigEndianUint32::make(0))
, d_schemaId()
, d_reserved()
{
    const int headerSizeWords = sizeof(PutHeader) / Protocol::k_WORD_SIZE;
    setHeaderWords(headerSizeWords);
    setMessageWords(headerSizeWords);
}

// MANIPULATORS
inline PutHeader& PutHeader::setFlags(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_FLAGS_NUM_BITS) - 1);

    d_flagsAndMessageWords = (d_flagsAndMessageWords & k_MSG_WORDS_MASK) |
                             (value << k_FLAGS_START_IDX);
    return *this;
}

inline PutHeader& PutHeader::setMessageWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_MSG_WORDS_NUM_BITS) - 1);

    d_flagsAndMessageWords = (d_flagsAndMessageWords & k_FLAGS_MASK) |
                             (value & k_MSG_WORDS_MASK);
    return *this;
}

inline PutHeader& PutHeader::setOptionsWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_OPTIONS_WORDS_NUM_BITS) - 1);

    d_optionsAndHeaderWordsAndCAT = (d_optionsAndHeaderWordsAndCAT &
                                     k_CAT_MASK) |
                                    (d_optionsAndHeaderWordsAndCAT &
                                     k_HEADER_WORDS_MASK) |
                                    (value << k_OPTIONS_WORDS_START_IDX);
    return *this;
}

inline PutHeader& PutHeader::setCompressionAlgorithmType(
    bmqt::CompressionAlgorithmType::Enum value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(
        value >= bmqt::CompressionAlgorithmType::k_LOWEST_SUPPORTED_TYPE &&
        value <= bmqt::CompressionAlgorithmType::k_HIGHEST_SUPPORTED_TYPE);

    d_optionsAndHeaderWordsAndCAT = (d_optionsAndHeaderWordsAndCAT &
                                     k_HEADER_WORDS_MASK) |
                                    (d_optionsAndHeaderWordsAndCAT &
                                     k_OPTIONS_WORDS_MASK) |
                                    (value << k_CAT_START_IDX);
    return *this;
}

inline PutHeader& PutHeader::setHeaderWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_HEADER_WORDS_NUM_BITS) - 1);

    d_optionsAndHeaderWordsAndCAT = (d_optionsAndHeaderWordsAndCAT &
                                     k_OPTIONS_WORDS_MASK) |
                                    (d_optionsAndHeaderWordsAndCAT &
                                     k_CAT_MASK) |
                                    (value & k_HEADER_WORDS_MASK);
    return *this;
}

inline PutHeader& PutHeader::setQueueId(int value)
{
    d_queueId = value;
    return *this;
}

inline PutHeader& PutHeader::setCorrelationId(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_CORRELATION_ID_NUM_BITS) - 1);

    bdlb::BigEndianInt32 corrIdBE = bdlb::BigEndianInt32::make(value);

    // Copy 3 lower bytes of 'corrIdBE' in d_guidOrCorrId[1-3]
    bsl::memcpy(&d_guidOrCorrId[1],
                (reinterpret_cast<char*>(&corrIdBE) + 1),  // skip MSB
                k_CORRELATION_ID_LEN);

    return *this;
}

inline PutHeader& PutHeader::setMessageGUID(const bmqt::MessageGUID& value)
{
    value.toBinary(&d_guidOrCorrId[0]);
    return *this;
}

inline PutHeader& PutHeader::setCrc32c(unsigned int value)
{
    d_crc32c = value;
    return *this;
}

inline SchemaWireId& PutHeader::schemaId()
{
    return d_schemaId;
}

// ACCESSORS
inline int PutHeader::flags() const
{
    return (d_flagsAndMessageWords & k_FLAGS_MASK) >> k_FLAGS_START_IDX;
}

inline int PutHeader::messageWords() const
{
    return d_flagsAndMessageWords & k_MSG_WORDS_MASK;
}

inline int PutHeader::optionsWords() const
{
    return (d_optionsAndHeaderWordsAndCAT & k_OPTIONS_WORDS_MASK) >>
           k_OPTIONS_WORDS_START_IDX;
}

inline bmqt::CompressionAlgorithmType::Enum
PutHeader::compressionAlgorithmType() const
{
    int value = (d_optionsAndHeaderWordsAndCAT & k_CAT_MASK) >>
                k_CAT_START_IDX;
    return static_cast<bmqt::CompressionAlgorithmType::Enum>(value);
}

inline int PutHeader::headerWords() const
{
    return d_optionsAndHeaderWordsAndCAT & k_HEADER_WORDS_MASK;
}

inline int PutHeader::queueId() const
{
    return d_queueId;
}

inline int PutHeader::correlationId() const
{
    // Retrieve correlationId from d_guidOrCorrId[1-3]
    bdlb::BigEndianInt32 corrIdBE = bdlb::BigEndianInt32::make(0);
    bsl::memcpy(reinterpret_cast<char*>(&corrIdBE) + 1,  // copy 3 LSB
                &d_guidOrCorrId[1],
                k_CORRELATION_ID_LEN);
    return corrIdBE;
}

inline const bmqt::MessageGUID& PutHeader::messageGUID() const
{
    return reinterpret_cast<const bmqt::MessageGUID&>(d_guidOrCorrId);
}

inline unsigned int PutHeader::crc32c() const
{
    return d_crc32c;
}

inline const SchemaWireId& PutHeader::schemaId() const
{
    return d_schemaId;
}

// ----------------
// struct AckHeader
// ----------------

// CREATORS
inline AckHeader::AckHeader()
{
    bsl::memset(this, 0, sizeof(AckHeader));
    setHeaderWords(sizeof(AckHeader) / Protocol::k_WORD_SIZE);
    setPerMessageWords(sizeof(AckMessage) / Protocol::k_WORD_SIZE);
}

// MANIPULATORS
inline AckHeader& AckHeader::setHeaderWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_HEADER_WORDS_NUM_BITS) - 1);

    d_headerWordsAndPerMsgWords = static_cast<unsigned char>(
        (d_headerWordsAndPerMsgWords & k_PER_MSG_WORDS_MASK) |
        (value << k_HEADER_WORDS_START_IDX));
    return *this;
}

inline AckHeader& AckHeader::setPerMessageWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_PER_MSG_WORDS_NUM_BITS) - 1);

    d_headerWordsAndPerMsgWords = static_cast<unsigned char>(
        (d_headerWordsAndPerMsgWords & k_HEADER_WORDS_MASK) |
        (value & k_PER_MSG_WORDS_MASK));
    return *this;
}

inline AckHeader& AckHeader::setFlags(unsigned char value)
{
    d_flags = value;
    return *this;
}

// ACCESSORS
inline int AckHeader::headerWords() const
{
    return (d_headerWordsAndPerMsgWords & k_HEADER_WORDS_MASK) >>
           k_HEADER_WORDS_START_IDX;
}

inline int AckHeader::perMessageWords() const
{
    return d_headerWordsAndPerMsgWords & k_PER_MSG_WORDS_MASK;
}

inline unsigned char AckHeader::flags() const
{
    return d_flags;
}

// -----------------
// struct AckMessage
// -----------------
// CREATORS
inline AckMessage::AckMessage()
{
    bsl::memset(this, 0, sizeof(AckMessage));
}

inline AckMessage::AckMessage(int                      status,
                              int                      correlationId,
                              const bmqt::MessageGUID& guid,
                              int                      queueId)
{
    d_statusAndCorrelationId = 0;
    // NOTE: status and correlationId are stored on the same field, so
    //       'setStatus' would be reading an 'uninitialized'
    //       'd_statusAndCorrelationId' field.

    setStatus(status);
    setCorrelationId(correlationId);
    setMessageGUID(guid);
    setQueueId(queueId);
}

// MANIPULATORS
inline AckMessage& AckMessage::setStatus(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_STATUS_NUM_BITS) - 1);

    d_statusAndCorrelationId = (d_statusAndCorrelationId & k_CORRID_MASK) |
                               (value << k_STATUS_START_IDX);
    return *this;
}

inline AckMessage& AckMessage::setCorrelationId(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_CORRID_NUM_BITS) - 1);

    d_statusAndCorrelationId = (d_statusAndCorrelationId & k_STATUS_MASK) |
                               (value & k_CORRID_MASK);
    return *this;
}

inline AckMessage& AckMessage::setMessageGUID(const bmqt::MessageGUID& value)
{
    value.toBinary(&d_messageGUID[0]);
    return *this;
}

inline AckMessage& AckMessage::setQueueId(int value)
{
    d_queueId = value;
    return *this;
}

// ACCESSORS
inline int AckMessage::status() const
{
    return (d_statusAndCorrelationId & k_STATUS_MASK) >> k_STATUS_START_IDX;
}

inline int AckMessage::correlationId() const
{
    return (d_statusAndCorrelationId & k_CORRID_MASK);
}

inline const bmqt::MessageGUID& AckMessage::messageGUID() const
{
    return reinterpret_cast<const bmqt::MessageGUID&>(d_messageGUID);
}

inline int AckMessage::queueId() const
{
    return d_queueId;
}

// -----------------
// struct PushHeader
// -----------------

// CREATORS
inline PushHeader::PushHeader()
: d_flagsAndMessageWords(bdlb::BigEndianUint32::make(0))
, d_optionsAndHeaderWordsAndCAT(bdlb::BigEndianUint32::make(0))
, d_queueId(bdlb::BigEndianInt32::make(0))
, d_messageGUID()
, d_schemaId()
, d_reserved()
{
    const int headerWords = sizeof(PushHeader) / Protocol::k_WORD_SIZE;
    setHeaderWords(headerWords);
    setMessageWords(headerWords);
}

// MANIPULATORS
inline PushHeader& PushHeader::setFlags(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_FLAGS_NUM_BITS) - 1);

    d_flagsAndMessageWords = (d_flagsAndMessageWords & k_MSG_WORDS_MASK) |
                             (value << k_FLAGS_START_IDX);
    return *this;
}

inline PushHeader& PushHeader::setMessageWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_MSG_WORDS_NUM_BITS) - 1);

    d_flagsAndMessageWords = (d_flagsAndMessageWords & k_FLAGS_MASK) |
                             (value << k_MSG_WORDS_START_IDX);
    return *this;
}

inline PushHeader& PushHeader::setOptionsWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_OPTIONS_WORDS_NUM_BITS) - 1);

    d_optionsAndHeaderWordsAndCAT = (d_optionsAndHeaderWordsAndCAT &
                                     k_CAT_MASK) |
                                    (d_optionsAndHeaderWordsAndCAT &
                                     k_HEADER_WORDS_MASK) |
                                    (value << k_OPTIONS_WORDS_START_IDX);
    return *this;
}

inline PushHeader& PushHeader::setCompressionAlgorithmType(
    bmqt::CompressionAlgorithmType::Enum value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(
        value >= bmqt::CompressionAlgorithmType::k_LOWEST_SUPPORTED_TYPE &&
        value <= bmqt::CompressionAlgorithmType::k_HIGHEST_SUPPORTED_TYPE);
    d_optionsAndHeaderWordsAndCAT = (d_optionsAndHeaderWordsAndCAT &
                                     k_OPTIONS_WORDS_MASK) |
                                    (d_optionsAndHeaderWordsAndCAT &
                                     k_HEADER_WORDS_MASK) |
                                    (value << k_CAT_START_IDX);
    return *this;
}

inline PushHeader& PushHeader::setHeaderWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_HEADER_WORDS_NUM_BITS) - 1);

    d_optionsAndHeaderWordsAndCAT = (d_optionsAndHeaderWordsAndCAT &
                                     k_OPTIONS_WORDS_MASK) |
                                    (d_optionsAndHeaderWordsAndCAT &
                                     k_CAT_MASK) |
                                    (value << k_HEADER_WORDS_START_IDX);
    return *this;
}

inline PushHeader& PushHeader::setQueueId(int value)
{
    d_queueId = value;
    return *this;
}

inline PushHeader& PushHeader::setMessageGUID(const bmqt::MessageGUID& value)
{
    value.toBinary(&d_messageGUID[0]);
    return *this;
}

inline SchemaWireId& PushHeader::schemaId()
{
    return d_schemaId;
}

// ACCESSORS
inline int PushHeader::flags() const
{
    return (d_flagsAndMessageWords & k_FLAGS_MASK) >> k_FLAGS_START_IDX;
}

inline int PushHeader::messageWords() const
{
    return d_flagsAndMessageWords & k_MSG_WORDS_MASK;
}

inline int PushHeader::optionsWords() const
{
    return (d_optionsAndHeaderWordsAndCAT & k_OPTIONS_WORDS_MASK) >>
           k_OPTIONS_WORDS_START_IDX;
}

inline bmqt::CompressionAlgorithmType::Enum
PushHeader::compressionAlgorithmType() const
{
    int value = (d_optionsAndHeaderWordsAndCAT & k_CAT_MASK) >>
                k_CAT_START_IDX;
    return static_cast<bmqt::CompressionAlgorithmType::Enum>(value);
}

inline int PushHeader::headerWords() const
{
    return d_optionsAndHeaderWordsAndCAT & k_HEADER_WORDS_MASK;
}

inline int PushHeader::queueId() const
{
    return d_queueId;
}

inline const bmqt::MessageGUID& PushHeader::messageGUID() const
{
    return reinterpret_cast<const bmqt::MessageGUID&>(d_messageGUID);
}

inline const SchemaWireId& PushHeader::schemaId() const
{
    return d_schemaId;
}

// -------------------------
// struct PushHeaderFlagUtil
// -------------------------

inline void PushHeaderFlagUtil::setFlag(int* flags, PushHeaderFlags::Enum flag)
{
    *flags = (*flags | flag);
}

inline void PushHeaderFlagUtil::unsetFlag(int*                  flags,
                                          PushHeaderFlags::Enum flag)
{
    *flags &= ~flag;
}

inline bool PushHeaderFlagUtil::isSet(int flags, PushHeaderFlags::Enum flag)
{
    return (0 != (flags & flag));
}

// --------------------
// struct ConfirmHeader
// --------------------

// CREATORS
inline ConfirmHeader::ConfirmHeader()
{
    bsl::memset(this, 0, sizeof(ConfirmHeader));
    setHeaderWords(sizeof(ConfirmHeader) / Protocol::k_WORD_SIZE);
    setPerMessageWords(sizeof(ConfirmMessage) / Protocol::k_WORD_SIZE);
}

// MANIPULATORS
inline ConfirmHeader& ConfirmHeader::setHeaderWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_HEADER_WORDS_NUM_BITS) - 1);

    d_headerWordsAndPerMsgWords = static_cast<unsigned char>(
        (d_headerWordsAndPerMsgWords & k_PER_MSG_WORDS_MASK) |
        (value << k_HEADER_WORDS_START_IDX));
    return *this;
}

inline ConfirmHeader& ConfirmHeader::setPerMessageWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_PER_MSG_WORDS_NUM_BITS) - 1);

    d_headerWordsAndPerMsgWords = static_cast<unsigned char>(
        (d_headerWordsAndPerMsgWords & k_HEADER_WORDS_MASK) |
        (value & k_PER_MSG_WORDS_MASK));
    return *this;
}

// ACCESSORS
inline int ConfirmHeader::headerWords() const
{
    return (d_headerWordsAndPerMsgWords & k_HEADER_WORDS_MASK) >>
           k_HEADER_WORDS_START_IDX;
}

inline int ConfirmHeader::perMessageWords() const
{
    return d_headerWordsAndPerMsgWords & k_PER_MSG_WORDS_MASK;
}

// ---------------------
// struct ConfirmMessage
// ---------------------
// CREATORS
inline ConfirmMessage::ConfirmMessage()
{
    bsl::memset(this, 0, sizeof(ConfirmMessage));
}

// MANIPULATORS
inline ConfirmMessage& ConfirmMessage::setQueueId(int value)
{
    d_queueId = value;
    return *this;
}

inline ConfirmMessage&
ConfirmMessage::setMessageGUID(const bmqt::MessageGUID& value)
{
    value.toBinary(&d_messageGUID[0]);
    return *this;
}

inline ConfirmMessage& ConfirmMessage::setSubQueueId(int value)
{
    d_subQueueId = value;
    return *this;
}

// ACCESSORS
inline int ConfirmMessage::queueId() const
{
    return d_queueId;
}

inline const bmqt::MessageGUID& ConfirmMessage::messageGUID() const
{
    return reinterpret_cast<const bmqt::MessageGUID&>(d_messageGUID);
}

inline int ConfirmMessage::subQueueId() const
{
    return d_subQueueId;
}

// -------------------
// struct RejectHeader
// -------------------

// CREATORS
inline RejectHeader::RejectHeader()
{
    bsl::memset(this, 0, sizeof(RejectHeader));
    setHeaderWords(sizeof(RejectHeader) / Protocol::k_WORD_SIZE);
    setPerMessageWords(sizeof(RejectMessage) / Protocol::k_WORD_SIZE);
}

// MANIPULATORS
inline RejectHeader& RejectHeader::setHeaderWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_HEADER_WORDS_NUM_BITS) - 1);

    d_headerWordsAndPerMsgWords = static_cast<unsigned char>(
        (d_headerWordsAndPerMsgWords & k_PER_MSG_WORDS_MASK) |
        (value << k_HEADER_WORDS_START_IDX));
    return *this;
}

inline RejectHeader& RejectHeader::setPerMessageWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_PER_MSG_WORDS_NUM_BITS) - 1);

    d_headerWordsAndPerMsgWords = static_cast<unsigned char>(
        (d_headerWordsAndPerMsgWords & k_HEADER_WORDS_MASK) |
        (value & k_PER_MSG_WORDS_MASK));
    return *this;
}

// ACCESSORS
inline int RejectHeader::headerWords() const
{
    return (d_headerWordsAndPerMsgWords & k_HEADER_WORDS_MASK) >>
           k_HEADER_WORDS_START_IDX;
}

inline int RejectHeader::perMessageWords() const
{
    return d_headerWordsAndPerMsgWords & k_PER_MSG_WORDS_MASK;
}

// --------------------
// struct RejectMessage
// --------------------

// CREATORS
inline RejectMessage::RejectMessage()
{
    bsl::memset(this, 0, sizeof(RejectMessage));
}

// MANIPULATORS
inline RejectMessage& RejectMessage::setQueueId(int value)
{
    d_queueId = value;
    return *this;
}

inline RejectMessage&
RejectMessage::setMessageGUID(const bmqt::MessageGUID& value)
{
    value.toBinary(&d_messageGUID[0]);
    return *this;
}

inline RejectMessage& RejectMessage::setSubQueueId(int value)
{
    d_subQueueId = value;
    return *this;
}

// ACCESSORS
inline int RejectMessage::queueId() const
{
    return d_queueId;
}

inline const bmqt::MessageGUID& RejectMessage::messageGUID() const
{
    return reinterpret_cast<const bmqt::MessageGUID&>(d_messageGUID);
}

inline int RejectMessage::subQueueId() const
{
    return d_subQueueId;
}

// -------------------------
// struct ReplicationReceipt
// -------------------------

// CREATORS
inline ReplicationReceipt::ReplicationReceipt()
{
    bsl::memset(this, 0, sizeof(ReplicationReceipt));
}

// MANIPULATORS
inline ReplicationReceipt&
ReplicationReceipt::setPartitionId(unsigned int value)
{
    d_partitionId = value;
    return *this;
}

inline ReplicationReceipt&
ReplicationReceipt::setSequenceNum(bsls::Types::Uint64 value)
{
    bmqp::Protocol::split(&d_seqNumUpperBits, &d_seqNumLowerBits, value);
    return *this;
}

inline ReplicationReceipt&
ReplicationReceipt::setPrimaryLeaseId(unsigned int value)
{
    d_primaryLeaseId = value;
    return *this;
}

// ACCESSORS
inline unsigned int ReplicationReceipt::partitionId() const
{
    return d_partitionId;
}

inline unsigned int ReplicationReceipt::primaryLeaseId() const
{
    return d_primaryLeaseId;
}

inline bsls::Types::Uint64 ReplicationReceipt::sequenceNum() const
{
    return bmqp::Protocol::combine(
        static_cast<unsigned int>(d_seqNumUpperBits),
        d_seqNumLowerBits);
}

// --------------------
// struct StorageHeader
// --------------------

// CREATORS
inline StorageHeader::StorageHeader()
{
    bsl::memset(this, 0, sizeof(StorageHeader));
    const int headerWords = sizeof(StorageHeader) / Protocol::k_WORD_SIZE;
    setMessageWords(headerWords);
    setHeaderWords(headerWords);
}

// MANIPULATORS
inline StorageHeader& StorageHeader::setFlags(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_FLAGS_NUM_BITS) - 1);

    d_flagsAndMessageWords = (d_flagsAndMessageWords & k_MSG_WORDS_MASK) |
                             (value << k_FLAGS_START_IDX);
    return *this;
}

inline StorageHeader& StorageHeader::setMessageWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_MSG_WORDS_NUM_BITS) - 1);

    d_flagsAndMessageWords = (d_flagsAndMessageWords & k_FLAGS_MASK) |
                             (value & k_MSG_WORDS_MASK);
    return *this;
}

inline StorageHeader& StorageHeader::setStorageProtocolVersion(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 && value <= (1 << k_SPV_NUM_BITS) - 1);

    d_reservedAndSpvAndHeaderWords = static_cast<unsigned char>(
        (d_reservedAndSpvAndHeaderWords & k_HEADER_WORDS_MASK) |
        (value << k_SPV_START_IDX));
    return *this;
}

inline StorageHeader& StorageHeader::setHeaderWords(unsigned int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value <= (1 << k_HEADER_WORDS_NUM_BITS) - 1);

    d_reservedAndSpvAndHeaderWords = static_cast<unsigned char>(
        (d_reservedAndSpvAndHeaderWords & k_SPV_MASK) |
        (value & k_HEADER_WORDS_MASK));
    return *this;
}

inline StorageHeader&
StorageHeader::setMessageType(StorageMessageType::Enum value)
{
    d_messageType = static_cast<unsigned char>(value);
    return *this;
}

inline StorageHeader& StorageHeader::setPartitionId(unsigned int value)
{
    d_partitionId = static_cast<unsigned short>(value);
    return *this;
}

inline StorageHeader& StorageHeader::setJournalOffsetWords(unsigned int value)
{
    d_journalOffsetWords = value;
    return *this;
}

// ACCESSORS
inline int StorageHeader::flags() const
{
    return (d_flagsAndMessageWords & k_FLAGS_MASK) >> k_FLAGS_START_IDX;
}

inline int StorageHeader::messageWords() const
{
    return d_flagsAndMessageWords & k_MSG_WORDS_MASK;
}

inline int StorageHeader::storageProtocolVersion() const
{
    return (d_reservedAndSpvAndHeaderWords & k_SPV_MASK) >> k_SPV_START_IDX;
}

inline int StorageHeader::headerWords() const
{
    return d_reservedAndSpvAndHeaderWords & k_HEADER_WORDS_MASK;
}

inline StorageMessageType::Enum StorageHeader::messageType() const
{
    return static_cast<StorageMessageType::Enum>(d_messageType);
}

inline unsigned int StorageHeader::partitionId() const
{
    return d_partitionId;
}

inline unsigned int StorageHeader::journalOffsetWords() const
{
    return d_journalOffsetWords;
}

// ----------------------------
// struct StorageHeaderFlagUtil
// ----------------------------

// CLASS METHODS
inline void StorageHeaderFlagUtil::setFlag(int*                     flags,
                                           StorageHeaderFlags::Enum flag)
{
    *flags = (*flags | flag);
}

inline void StorageHeaderFlagUtil::unsetFlag(int*                     flags,
                                             StorageHeaderFlags::Enum flag)
{
    *flags &= ~flag;
}

inline bool StorageHeaderFlagUtil::isSet(unsigned char            flags,
                                         StorageHeaderFlags::Enum flag)
{
    return (0 != (flags & flag));
}

// ---------------------
// struct RecoveryHeader
// ---------------------

// CREATORS
inline RecoveryHeader::RecoveryHeader()
{
    bsl::memset(this, 0, sizeof(RecoveryHeader));
    const int headerWords = sizeof(RecoveryHeader) / Protocol::k_WORD_SIZE;
    setMessageWords(headerWords);
    setHeaderWords(headerWords);
}

// MANIPULATORS
inline RecoveryHeader& RecoveryHeader::setFinalChunkBit()
{
    d_fcbAndReservedAndMessageWords = (d_fcbAndReservedAndMessageWords &
                                       k_MSG_WORDS_MASK) |
                                      (1U << k_FCB_START_IDX);
    return *this;
}

inline RecoveryHeader& RecoveryHeader::setMessageWords(int value)
{
    BSLS_ASSERT_SAFE(value >= 0);

    d_fcbAndReservedAndMessageWords = (d_fcbAndReservedAndMessageWords &
                                       k_FCB_MASK) |
                                      (value & k_MSG_WORDS_MASK);
    return *this;
}

inline RecoveryHeader& RecoveryHeader::setHeaderWords(int value)
{
    // PRECONDITIONS: protect against overflow
    BSLS_ASSERT_SAFE(value >= 0 &&
                     value <= (1 << k_HEADER_WORDS_NUM_BITS) - 1);

    d_headerWordsAndFileChunkType = static_cast<unsigned char>(
        (d_headerWordsAndFileChunkType & k_FILE_CHUNK_TYPE_MASK) |
        (value << k_HEADER_WORDS_START_IDX));

    return *this;
}

inline RecoveryHeader&
RecoveryHeader::setFileChunkType(RecoveryFileChunkType::Enum value)
{
    d_headerWordsAndFileChunkType = static_cast<unsigned char>(
        (d_headerWordsAndFileChunkType & k_HEADER_WORDS_MASK) |
        (value & k_FILE_CHUNK_TYPE_MASK));
    return *this;
}

inline RecoveryHeader& RecoveryHeader::setPartitionId(unsigned int value)
{
    d_partitionId = static_cast<unsigned short>(value);
    return *this;
}

inline RecoveryHeader&
RecoveryHeader::setChunkSequenceNumber(unsigned int value)
{
    d_chunkSequenceNumber = value;
    return *this;
}

inline RecoveryHeader& RecoveryHeader::setMd5Digest(const char* value)
{
    bsl::memcpy(d_md5Digest, value, k_MD5_DIGEST_LEN);
    return *this;
}

// ACCESSORS
inline bool RecoveryHeader::isFinalChunk() const
{
    return 1 ==
           ((d_fcbAndReservedAndMessageWords & k_FCB_MASK) >> k_FCB_START_IDX);
}

inline int RecoveryHeader::messageWords() const
{
    return d_fcbAndReservedAndMessageWords & k_MSG_WORDS_MASK;
}

inline int RecoveryHeader::headerWords() const
{
    return ((d_headerWordsAndFileChunkType & k_HEADER_WORDS_MASK) >>
            k_HEADER_WORDS_START_IDX);
}

inline RecoveryFileChunkType::Enum RecoveryHeader::fileChunkType() const
{
    return static_cast<RecoveryFileChunkType::Enum>(
        d_headerWordsAndFileChunkType & k_FILE_CHUNK_TYPE_MASK);
}

inline unsigned int RecoveryHeader::partitionId() const
{
    return d_partitionId;
}

inline unsigned int RecoveryHeader::chunkSequenceNumber() const
{
    return d_chunkSequenceNumber;
}

inline const char* RecoveryHeader::md5Digest() const
{
    return d_md5Digest;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& bmqp::operator<<(bsl::ostream&         stream,
                                      bmqp::EventType::Enum value)
{
    return bmqp::EventType::print(stream, value, 0, -1);
}

inline bsl::ostream& bmqp::operator<<(bsl::ostream&            stream,
                                      bmqp::EncodingType::Enum value)
{
    return bmqp::EncodingType::print(stream, value, 0, -1);
}

inline bsl::ostream& bmqp::operator<<(bsl::ostream&          stream,
                                      bmqp::OptionType::Enum value)
{
    return bmqp::OptionType::print(stream, value, 0, -1);
}

inline bsl::ostream& bmqp::operator<<(bsl::ostream& stream, const RdaInfo& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bsl::ostream& bmqp::operator<<(bsl::ostream&       stream,
                                      const SubQueueInfo& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bool bmqp::operator==(const RdaInfo& lhs, const RdaInfo& rhs)
{
    return rhs.d_counter == lhs.d_counter;
}

inline bool bmqp::operator==(const SubQueueInfo& lhs, const SubQueueInfo& rhs)
{
    return rhs.id() == lhs.id() && rhs.rdaInfo() == lhs.rdaInfo();
}

inline bsl::ostream& bmqp::operator<<(bsl::ostream&              stream,
                                      bmqp::PutHeaderFlags::Enum value)
{
    return bmqp::PutHeaderFlags::print(stream, value, 0, -1);
}

inline bsl::ostream& bmqp::operator<<(bsl::ostream&               stream,
                                      bmqp::PushHeaderFlags::Enum value)
{
    return bmqp::PushHeaderFlags::print(stream, value, 0, -1);
}

inline bsl::ostream& bmqp::operator<<(bsl::ostream&                  stream,
                                      bmqp::StorageMessageType::Enum value)
{
    return bmqp::StorageMessageType::print(stream, value, 0, -1);
}

inline bsl::ostream& bmqp::operator<<(bsl::ostream&                  stream,
                                      bmqp::StorageHeaderFlags::Enum value)
{
    return bmqp::StorageHeaderFlags::print(stream, value, 0, -1);
}

inline bsl::ostream& bmqp::operator<<(bsl::ostream&                     stream,
                                      bmqp::RecoveryFileChunkType::Enum value)
{
    return bmqp::RecoveryFileChunkType::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
