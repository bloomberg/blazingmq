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

// bmqp_protocol.cpp                                                  -*-C++-*-
#include <bmqp_protocol.h>

#include <bmqscm_version.h>
// BDE
#include <bdlb_bitmaskutil.h>
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdlb_tokenizer.h>
#include <bsl_cstddef.h>
#include <bsl_limits.h>
#include <bslim_printer.h>
#include <bslmf_assert.h>
#include <bsls_alignmentfromtype.h>

namespace BloombergLP {
namespace bmqp {

namespace {

// Compile-time assertions for the range of encodings supported currently
BSLMF_ASSERT(EncodingType::e_BER ==
             EncodingType::k_LOWEST_SUPPORTED_ENCODING_TYPE);
BSLMF_ASSERT(EncodingType::e_JSON ==
             EncodingType::k_HIGHEST_SUPPORTED_ENCODING_TYPE);

// Compile-time assertions for alignment of various headers in the protocol
BSLMF_ASSERT(1 == bsls::AlignmentFromType<RdaInfo>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<SubQueueInfo>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<EventHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<OptionHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<PutHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<AckMessage>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<PushHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<ConfirmMessage>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<RejectMessage>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<StorageHeader>::VALUE);
BSLMF_ASSERT(4 == bsls::AlignmentFromType<RecoveryHeader>::VALUE);
BSLMF_ASSERT(2 == bsls::AlignmentFromType<MessagePropertiesHeader>::VALUE);
BSLMF_ASSERT(2 == bsls::AlignmentFromType<MessagePropertyHeader>::VALUE);

// These three headers are 1-byte aligned but headers and messages preceding &
// succeeding them are 4-byte aligned.  So as long as their size is 4 (or a
// multiple of 4), it is ok.
BSLMF_ASSERT(1 == bsls::AlignmentFromType<AckHeader>::VALUE);
BSLMF_ASSERT(1 == bsls::AlignmentFromType<ConfirmHeader>::VALUE);
BSLMF_ASSERT(1 == bsls::AlignmentFromType<RejectHeader>::VALUE);
BSLMF_ASSERT(4 == sizeof(AckHeader));
BSLMF_ASSERT(4 == sizeof(ConfirmHeader));
BSLMF_ASSERT(4 == sizeof(RejectHeader));

BSLMF_ASSERT(PushHeader::k_MAX_PAYLOAD_SIZE_SOFT ==
             PutHeader::k_MAX_PAYLOAD_SIZE_SOFT);

BSLMF_ASSERT(RecoveryHeader::k_MAX_PAYLOAD_SIZE_SOFT <=
             RecoveryHeader::k_MAX_SIZE);

BSLMF_ASSERT(Protocol::SubQueueInfosArray::static_size ==
             Protocol::k_SUBID_ARRAY_STATIC_LEN);
// Ensure that the static size of a subQueueInfos array is as defined in
// the designated constant

}  // close unnamed namespace

// ==============
// struct RdaInfo
// ==============

const unsigned char RdaInfo::k_MAX_COUNTER_VALUE = 63U;

const unsigned char RdaInfo::k_MAX_INTERNAL_COUNTER_VALUE = 255U;

bsl::ostream&
RdaInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    if (isUnlimited()) {
        printer.printAttribute("isUnlimited", isUnlimited());
    }
    else {
        printer.printAttribute("counter", counter());
        printer.printAttribute("isUnlimited", isUnlimited());
        printer.printAttribute("isPotentiallyPoisonous",
                               isPotentiallyPoisonous());
    }
    printer.end();

    return stream;
}

// -------------------
// struct SubQueueInfo
// -------------------

// CREATORS
SubQueueInfo::SubQueueInfo()
{
    bsl::memset(d_reserved, 0, sizeof(d_reserved));
    setId(bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID);
}

SubQueueInfo::SubQueueInfo(unsigned int id)
{
    bsl::memset(d_reserved, 0, sizeof(d_reserved));
    setId(id);
}

SubQueueInfo::SubQueueInfo(unsigned int id, const RdaInfo& rdaInfo)
: d_rdaInfo(rdaInfo)
{
    bsl::memset(d_reserved, 0, sizeof(d_reserved));
    setId(id);
}

bsl::ostream&
SubQueueInfo::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("subQueueId", d_subQueueId);
    printer.printAttribute("rdaCounter", d_rdaInfo);
    printer.end();

    return stream;
}

// ---------------
// struct Protocol
// ---------------

const int Protocol::k_MAX_OPTIONS_SIZE = PutHeader::k_MAX_OPTIONS_SIZE;

BSLMF_ASSERT(PutHeader::k_MAX_OPTIONS_SIZE == PushHeader::k_MAX_OPTIONS_SIZE);
BSLMF_ASSERT(PutHeader::k_MAX_OPTIONS_SIZE == Protocol::k_MAX_OPTIONS_SIZE);
// Ensure that maximum allowed size of options is same for both PUT and
// PUSH messages.  Having different values for these constants doesn't
// make much sense.

const int Protocol::k_CONSUMER_PRIORITY_INVALID =
    bsl::numeric_limits<int>::min();
const int Protocol::k_CONSUMER_PRIORITY_MIN = bsl::numeric_limits<int>::min() /
                                              2;
const int Protocol::k_CONSUMER_PRIORITY_MAX = bsl::numeric_limits<int>::max() /
                                              2;

// ----------------
// struct EventType
// ----------------

bsl::ostream& EventType::print(bsl::ostream&   stream,
                               EventType::Enum value,
                               int             level,
                               int             spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << EventType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* EventType::toAscii(EventType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(CONTROL)
        CASE(PUT)
        CASE(CONFIRM)
        CASE(REJECT)
        CASE(PUSH)
        CASE(ACK)
        CASE(CLUSTER_STATE)
        CASE(ELECTOR)
        CASE(STORAGE)
        CASE(RECOVERY)
        CASE(PARTITION_SYNC)
        CASE(HEARTBEAT_REQ)
        CASE(HEARTBEAT_RSP)
        CASE(REPLICATION_RECEIPT)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// -------------------
// struct EncodingType
// -------------------

bsl::ostream& EncodingType::print(bsl::ostream&      stream,
                                  EncodingType::Enum value,
                                  int                level,
                                  int                spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << EncodingType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* EncodingType::toAscii(EncodingType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNKNOWN)
        CASE(BER)
        CASE(JSON)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ----------------------
// struct EncodingFeature
// ----------------------

const char EncodingFeature::k_FIELD_NAME[]    = "PROTOCOL_ENCODING";
const char EncodingFeature::k_ENCODING_BER[]  = "BER";
const char EncodingFeature::k_ENCODING_JSON[] = "JSON";

// -------------------------------
// struct HighAvailabilityFeatures
// -------------------------------

const char HighAvailabilityFeatures::k_FIELD_NAME[] = "HA";
const char HighAvailabilityFeatures::k_BROADCAST_TO_PROXIES[] =
    "BROADCAST_TO_PROXIES";
const char HighAvailabilityFeatures::k_GRACEFUL_SHUTDOWN[] =
    "GRACEFUL_SHUTDOWN";

// --------------------------------
// struct MessagePropertiesFeatures
// --------------------------------

const char MessagePropertiesFeatures::k_FIELD_NAME[] = "MPS";
const char MessagePropertiesFeatures::k_MESSAGE_PROPERTIES_EX[] =
    "MESSAGE_PROPERTIES_EX";

// -----------------
// struct OptionType
// -----------------

bsl::ostream& OptionType::print(bsl::ostream&    stream,
                                OptionType::Enum value,
                                int              level,
                                int              spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << OptionType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* OptionType::toAscii(OptionType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(SUB_QUEUE_IDS_OLD)
        CASE(MSG_GROUP_ID)
        CASE(SUB_QUEUE_INFOS)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ------------------
// struct EventHeader
// ------------------

const int EventHeader::k_FRAGMENT_MASK = bdlb::BitMaskUtil::one(
    EventHeader::k_FRAGMENT_START_IDX,
    EventHeader::k_FRAGMENT_NUM_BITS);

const int EventHeader::k_LENGTH_MASK = bdlb::BitMaskUtil::one(
    EventHeader::k_LENGTH_START_IDX,
    EventHeader::k_LENGTH_NUM_BITS);

const int EventHeader::k_PROTOCOL_VERSION_MASK = bdlb::BitMaskUtil::one(
    EventHeader::k_PROTOCOL_VERSION_START_IDX,
    EventHeader::k_PROTOCOL_VERSION_NUM_BITS);

const int EventHeader::k_TYPE_MASK = bdlb::BitMaskUtil::one(
    EventHeader::k_TYPE_START_IDX,
    EventHeader::k_TYPE_NUM_BITS);

// ----------------------
// struct EventHeaderUtil
// ----------------------

const int EventHeaderUtil::k_CONTROL_EVENT_ENCODING_MASK =
    bdlb::BitMaskUtil::one(EventHeaderUtil::k_CONTROL_EVENT_ENCODING_START_IDX,
                           EventHeaderUtil::k_CONTROL_EVENT_ENCODING_NUM_BITS);

// -------------------
// struct OptionHeader
// -------------------

const int OptionHeader::k_TYPE_MASK = bdlb::BitMaskUtil::one(
    OptionHeader::k_TYPE_START_IDX,
    OptionHeader::k_TYPE_NUM_BITS);

const int OptionHeader::k_PACKED_MASK = bdlb::BitMaskUtil::one(
    OptionHeader::k_PACKED_START_IDX,
    OptionHeader::k_PACKED_NUM_BITS);

const int OptionHeader::k_TYPE_SPECIFIC_MASK = bdlb::BitMaskUtil::one(
    OptionHeader::k_TYPE_SPECIFIC_START_IDX,
    OptionHeader::k_TYPE_SPECIFIC_NUM_BITS);

const int OptionHeader::k_WORDS_MASK = bdlb::BitMaskUtil::one(
    OptionHeader::k_WORDS_START_IDX,
    OptionHeader::k_WORDS_NUM_BITS);

// ------------------------------
// struct MessagePropertiesHeader
// ------------------------------

const int MessagePropertiesHeader::k_HEADER_SIZE_2X_MASK =
    bdlb::BitMaskUtil::one(MessagePropertiesHeader::k_HEADER_SIZE_2X_START_IDX,
                           MessagePropertiesHeader::k_HEADER_SIZE_2X_NUM_BITS);

const int MessagePropertiesHeader::k_MPH_SIZE_2X_MASK = bdlb::BitMaskUtil::one(
    MessagePropertiesHeader::k_MPH_SIZE_2X_START_IDX,
    MessagePropertiesHeader::k_MPH_SIZE_2X_NUM_BITS);

// ----------------------------
// struct MessagePropertyHeader
// ----------------------------

const int MessagePropertyHeader::k_PROP_TYPE_MASK = bdlb::BitMaskUtil::one(
    MessagePropertyHeader::k_PROP_TYPE_START_IDX,
    MessagePropertyHeader::k_PROP_TYPE_NUM_BITS);

const int MessagePropertyHeader::k_PROP_VALUE_LEN_UPPER_MASK =
    bdlb::BitMaskUtil::one(
        MessagePropertyHeader::k_PROP_VALUE_LEN_UPPER_START_IDX,
        MessagePropertyHeader::k_PROP_VALUE_LEN_UPPER_NUM_BITS);

const int MessagePropertyHeader::k_PROP_NAME_LEN_MASK = bdlb::BitMaskUtil::one(
    MessagePropertyHeader::k_PROP_NAME_LEN_START_IDX,
    MessagePropertyHeader::k_PROP_NAME_LEN_NUM_BITS);

// ----------------
// struct PutHeader
// ----------------

const int PutHeader::k_MAX_OPTIONS_SIZE;
const int PutHeader::k_MAX_PAYLOAD_SIZE_SOFT;
const int PutHeader::k_MAX_SIZE_SOFT;
// Force variable/symbol definition so that it can be used in other files

const int PutHeader::k_FLAGS_MASK = bdlb::BitMaskUtil::one(
    PutHeader::k_FLAGS_START_IDX,
    PutHeader::k_FLAGS_NUM_BITS);

const int PutHeader::k_MSG_WORDS_MASK = bdlb::BitMaskUtil::one(
    PutHeader::k_MSG_WORDS_START_IDX,
    PutHeader::k_MSG_WORDS_NUM_BITS);

const int PutHeader::k_OPTIONS_WORDS_MASK = bdlb::BitMaskUtil::one(
    PutHeader::k_OPTIONS_WORDS_START_IDX,
    PutHeader::k_OPTIONS_WORDS_NUM_BITS);

const int PutHeader::k_CAT_MASK = bdlb::BitMaskUtil::one(
    PutHeader::k_CAT_START_IDX,
    PutHeader::k_CAT_NUM_BITS);

const int PutHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    PutHeader::k_HEADER_WORDS_START_IDX,
    PutHeader::k_HEADER_WORDS_NUM_BITS);

// ---------------------
// struct PutHeaderFlags
// ---------------------

bsl::ostream& PutHeaderFlags::print(bsl::ostream&        stream,
                                    PutHeaderFlags::Enum value,
                                    int                  level,
                                    int                  spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << PutHeaderFlags::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* PutHeaderFlags::toAscii(PutHeaderFlags::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(ACK_REQUESTED)
        CASE(MESSAGE_PROPERTIES)
        CASE(UNUSED3)
        CASE(UNUSED4)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool PutHeaderFlags::fromAscii(PutHeaderFlags::Enum*    out,
                               const bslstl::StringRef& str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(PutHeaderFlags::e_##M),        \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = PutHeaderFlags::e_##M;                                         \
        return true;                                                          \
    }

    CHECKVALUE(ACK_REQUESTED)
    CHECKVALUE(MESSAGE_PROPERTIES)
    CHECKVALUE(UNUSED3)
    CHECKVALUE(UNUSED4)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// ------------------------
// struct PutHeaderFlagUtil
// ------------------------

bool PutHeaderFlagUtil::isValid(bsl::ostream& errorDescription, int flags)
{
    if (isSet(flags, PutHeaderFlags::e_UNUSED3) ||
        isSet(flags, PutHeaderFlags::e_UNUSED4)) {
        errorDescription << "UNUSED flags are invalid.";
        return false;  // RETURN
    }

    return true;
}

bsl::ostream& PutHeaderFlagUtil::prettyPrint(bsl::ostream& stream, int flags)
{
#define CHECKVALUE(M)                                                         \
    if (flags & PutHeaderFlags::e_##M) {                                      \
        stream << (first ? "" : ",")                                          \
               << PutHeaderFlags::toAscii(PutHeaderFlags::e_##M);             \
        first = false;                                                        \
    }

    bool first = true;

    CHECKVALUE(ACK_REQUESTED)
    CHECKVALUE(MESSAGE_PROPERTIES)
    CHECKVALUE(UNUSED3)
    CHECKVALUE(UNUSED4)

    return stream;

#undef CHECKVALUE
}

int PutHeaderFlagUtil::fromString(bsl::ostream&      errorDescription,
                                  int*               out,
                                  const bsl::string& str)
{
    int rc = 0;
    *out   = 0;

    bdlb::Tokenizer tokenizer(str, ",");
    for (bdlb::TokenizerIterator it = tokenizer.begin(); it != tokenizer.end();
         ++it) {
        PutHeaderFlags::Enum value;
        if (PutHeaderFlags::fromAscii(&value, *it) == false) {
            if (rc == 0) {  // First wrong flag
                errorDescription << "Invalid flag(s) '" << *it << "'";
            }
            else {
                errorDescription << ",'" << *it << "'";
            }
            rc = -1;
        }
        else {
            *out |= value;
        }
    }

    return rc;
}

// ----------------
// struct AckHeader
// ----------------

const int AckHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    AckHeader::k_HEADER_WORDS_START_IDX,
    AckHeader::k_HEADER_WORDS_NUM_BITS);

const int AckHeader::k_PER_MSG_WORDS_MASK = bdlb::BitMaskUtil::one(
    AckHeader::k_PER_MSG_WORDS_START_IDX,
    AckHeader::k_PER_MSG_WORDS_NUM_BITS);

// -----------------
// struct AckMessage
// -----------------

const int AckMessage::k_NULL_CORRELATION_ID;

const int AckMessage::k_STATUS_MASK = bdlb::BitMaskUtil::one(
    AckMessage::k_STATUS_START_IDX,
    AckMessage::k_STATUS_NUM_BITS);

const int AckMessage::k_CORRID_MASK = bdlb::BitMaskUtil::one(
    AckMessage::k_CORRID_START_IDX,
    AckMessage::k_CORRID_NUM_BITS);

// -----------------
// struct PushHeader
// -----------------

const int PushHeader::k_MAX_OPTIONS_SIZE;
// Force variable/symbol definition so that it can be used in other files

const int PushHeader::k_FLAGS_MASK = bdlb::BitMaskUtil::one(
    PushHeader::k_FLAGS_START_IDX,
    PushHeader::k_FLAGS_NUM_BITS);

const int PushHeader::k_MSG_WORDS_MASK = bdlb::BitMaskUtil::one(
    PushHeader::k_MSG_WORDS_START_IDX,
    PushHeader::k_MSG_WORDS_NUM_BITS);

const int PushHeader::k_OPTIONS_WORDS_MASK = bdlb::BitMaskUtil::one(
    PushHeader::k_OPTIONS_WORDS_START_IDX,
    PushHeader::k_OPTIONS_WORDS_NUM_BITS);

const int PushHeader::k_CAT_MASK = bdlb::BitMaskUtil::one(
    PushHeader::k_CAT_START_IDX,
    PushHeader::k_CAT_NUM_BITS);

const int PushHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    PushHeader::k_HEADER_WORDS_START_IDX,
    PushHeader::k_HEADER_WORDS_NUM_BITS);

// ----------------------
// struct PushHeaderFlags
// ----------------------

bsl::ostream& PushHeaderFlags::print(bsl::ostream&         stream,
                                     PushHeaderFlags::Enum value,
                                     int                   level,
                                     int                   spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << PushHeaderFlags::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* PushHeaderFlags::toAscii(PushHeaderFlags::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(IMPLICIT_PAYLOAD)
        CASE(MESSAGE_PROPERTIES)
        CASE(UNUSED3)
        CASE(UNUSED4)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool PushHeaderFlags::fromAscii(PushHeaderFlags::Enum*   out,
                                const bslstl::StringRef& str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(PushHeaderFlags::e_##M),       \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = PushHeaderFlags::e_##M;                                        \
        return true;                                                          \
    }

    CHECKVALUE(IMPLICIT_PAYLOAD)
    CHECKVALUE(MESSAGE_PROPERTIES)
    CHECKVALUE(UNUSED3)
    CHECKVALUE(UNUSED4)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// -------------------------
// struct PushHeaderFlagUtil
// -------------------------

bool PushHeaderFlagUtil::isValid(bsl::ostream& errorDescription, int flags)
{
    if (isSet(flags, PushHeaderFlags::e_UNUSED3) ||
        isSet(flags, PushHeaderFlags::e_UNUSED4)) {
        errorDescription << "UNUSED flags are invalid.";
        return false;  // RETURN
    }

    return true;
}

bsl::ostream& PushHeaderFlagUtil::prettyPrint(bsl::ostream& stream, int flags)
{
#define CHECKVALUE(M)                                                         \
    if (flags & PushHeaderFlags::e_##M) {                                     \
        stream << (first ? "" : ",")                                          \
               << PushHeaderFlags::toAscii(PushHeaderFlags::e_##M);           \
        first = false;                                                        \
    }

    bool first = true;

    CHECKVALUE(IMPLICIT_PAYLOAD)
    CHECKVALUE(MESSAGE_PROPERTIES)
    CHECKVALUE(UNUSED3)
    CHECKVALUE(UNUSED4)

    return stream;

#undef CHECKVALUE
}

int PushHeaderFlagUtil::fromString(bsl::ostream&      errorDescription,
                                   int*               out,
                                   const bsl::string& str)
{
    int rc = 0;
    *out   = 0;

    bdlb::Tokenizer tokenizer(str, ",");
    for (bdlb::TokenizerIterator it = tokenizer.begin(); it != tokenizer.end();
         ++it) {
        PushHeaderFlags::Enum value;
        if (PushHeaderFlags::fromAscii(&value, *it) == false) {
            if (rc == 0) {  // First wrong flag
                errorDescription << "Invalid flag(s) '" << *it << "'";
            }
            else {
                errorDescription << ",'" << *it << "'";
            }
            rc = -1;
        }
        else {
            *out |= value;
        }
    }

    return rc;
}

// --------------------
// struct ConfirmHeader
// --------------------

const int ConfirmHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    ConfirmHeader::k_HEADER_WORDS_START_IDX,
    ConfirmHeader::k_HEADER_WORDS_NUM_BITS);

const int ConfirmHeader::k_PER_MSG_WORDS_MASK = bdlb::BitMaskUtil::one(
    ConfirmHeader::k_PER_MSG_WORDS_START_IDX,
    ConfirmHeader::k_PER_MSG_WORDS_NUM_BITS);

// -------------------
// struct RejectHeader
// -------------------

const int RejectHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    RejectHeader::k_HEADER_WORDS_START_IDX,
    RejectHeader::k_HEADER_WORDS_NUM_BITS);

const int RejectHeader::k_PER_MSG_WORDS_MASK = bdlb::BitMaskUtil::one(
    RejectHeader::k_PER_MSG_WORDS_START_IDX,
    RejectHeader::k_PER_MSG_WORDS_NUM_BITS);

// --------------------
// struct StorageHeader
// --------------------

const unsigned int StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT;

const int StorageHeader::k_FLAGS_MASK = bdlb::BitMaskUtil::one(
    StorageHeader::k_FLAGS_START_IDX,
    StorageHeader::k_FLAGS_NUM_BITS);

const int StorageHeader::k_MSG_WORDS_MASK = bdlb::BitMaskUtil::one(
    StorageHeader::k_MSG_WORDS_START_IDX,
    StorageHeader::k_MSG_WORDS_NUM_BITS);

const int StorageHeader::k_SPV_MASK = bdlb::BitMaskUtil::one(
    StorageHeader::k_SPV_START_IDX,
    StorageHeader::k_SPV_NUM_BITS);

const int StorageHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    StorageHeader::k_HEADER_WORDS_START_IDX,
    StorageHeader::k_HEADER_WORDS_NUM_BITS);

// -------------------------
// struct StorageMessageType
// -------------------------

bsl::ostream& StorageMessageType::print(bsl::ostream&            stream,
                                        StorageMessageType::Enum value,
                                        int                      level,
                                        int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << StorageMessageType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* StorageMessageType::toAscii(StorageMessageType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(DATA)
        CASE(QLIST)
        CASE(CONFIRM)
        CASE(DELETION)
        CASE(JOURNAL_OP)
        CASE(QUEUE_OP)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// -------------------------
// struct StorageHeaderFlags
// -------------------------

bsl::ostream& StorageHeaderFlags::print(bsl::ostream&            stream,
                                        StorageHeaderFlags::Enum value,
                                        int                      level,
                                        int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << StorageHeaderFlags::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* StorageHeaderFlags::toAscii(StorageHeaderFlags::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(RECEIPT_REQUESTED)
        CASE(UNUSED2)
        CASE(UNUSED3)
        CASE(UNUSED4)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool StorageHeaderFlags::fromAscii(StorageHeaderFlags::Enum* out,
                                   const bslstl::StringRef&  str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(StorageHeaderFlags::e_##M),    \
                                       str.data(),                            \
                                       str.length())) {                       \
        *out = StorageHeaderFlags::e_##M;                                     \
        return true;                                                          \
    }

    CHECKVALUE(RECEIPT_REQUESTED)
    CHECKVALUE(UNUSED2)
    CHECKVALUE(UNUSED3)
    CHECKVALUE(UNUSED4)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// ----------------------------
// struct StorageHeaderFlagUtil
// ----------------------------

bool StorageHeaderFlagUtil::isValid(bsl::ostream& errorDescription,
                                    unsigned char flags)
{
    if (isSet(flags, StorageHeaderFlags::e_UNUSED2) ||
        isSet(flags, StorageHeaderFlags::e_UNUSED3) ||
        isSet(flags, StorageHeaderFlags::e_UNUSED4)) {
        errorDescription << "UNUSED flags are invalid.";
        return false;  // RETURN
    }

    return true;
}

bsl::ostream& StorageHeaderFlagUtil::prettyPrint(bsl::ostream& stream,
                                                 unsigned char flags)
{
#define CHECKVALUE(M)                                                         \
    if (flags & StorageHeaderFlags::e_##M) {                                  \
        stream << (first ? "" : ",")                                          \
               << StorageHeaderFlags::toAscii(StorageHeaderFlags::e_##M);     \
        first = false;                                                        \
    }

    bool first = true;

    CHECKVALUE(RECEIPT_REQUESTED)
    CHECKVALUE(UNUSED2)
    CHECKVALUE(UNUSED3)
    CHECKVALUE(UNUSED4)

    return stream;

#undef CHECKVALUE
}

int StorageHeaderFlagUtil::fromString(bsl::ostream&      errorDescription,
                                      unsigned char*     out,
                                      const bsl::string& str)
{
    int rc = 0;
    *out   = 0;

    bdlb::Tokenizer tokenizer(str, ",");
    for (bdlb::TokenizerIterator it = tokenizer.begin(); it != tokenizer.end();
         ++it) {
        StorageHeaderFlags::Enum value;
        if (StorageHeaderFlags::fromAscii(&value, *it) == false) {
            if (rc == 0) {  // First wrong flag
                errorDescription << "Invalid flag(s) '" << *it << "'";
            }
            else {
                errorDescription << ",'" << *it << "'";
            }
            rc = -1;
        }
        else {
            *out |= value;
        }
    }

    return rc;
}

// ----------------------------
// struct RecoveryFileChunkType
// ----------------------------

bsl::ostream& RecoveryFileChunkType::print(bsl::ostream&               stream,
                                           RecoveryFileChunkType::Enum value,
                                           int                         level,
                                           int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << RecoveryFileChunkType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* RecoveryFileChunkType::toAscii(RecoveryFileChunkType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(DATA)
        CASE(JOURNAL)
        CASE(QLIST)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ---------------------
// struct RecoveryHeader
// ---------------------

const int RecoveryHeader::k_FCB_MASK = bdlb::BitMaskUtil::one(
    RecoveryHeader::k_FCB_START_IDX,
    RecoveryHeader::k_FCB_NUM_BITS);

const int RecoveryHeader::k_MSG_WORDS_MASK = bdlb::BitMaskUtil::one(
    RecoveryHeader::k_MSG_WORDS_START_IDX,
    RecoveryHeader::k_MSG_WORDS_NUM_BITS);

const int RecoveryHeader::k_HEADER_WORDS_MASK = bdlb::BitMaskUtil::one(
    RecoveryHeader::k_HEADER_WORDS_START_IDX,
    RecoveryHeader::k_HEADER_WORDS_NUM_BITS);

const int RecoveryHeader::k_FILE_CHUNK_TYPE_MASK = bdlb::BitMaskUtil::one(
    RecoveryHeader::k_FILE_CHUNK_TYPE_START_IDX,
    RecoveryHeader::k_FILE_CHUNK_TYPE_NUM_BITS);

}  // close package namespace
}  // close enterprise namespace
