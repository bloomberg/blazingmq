// mqbconfm_messages.cpp           *DO NOT EDIT*           @generated -*-C++-*-

#include <mqbconfm_messages.h>

#include <bdlat_formattingmode.h>
#include <bdlat_valuetypefunctions.h>
#include <bdlb_print.h>
#include <bdlb_printmethods.h>
#include <bdlb_string.h>

#include <bdlb_nullablevalue.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslim_printer.h>
#include <bsls_assert.h>
#include <bsls_types.h>

#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bsl_utility.h>

namespace BloombergLP {
namespace mqbconfm {

// --------------------
// class BrokerIdentity
// --------------------

// CONSTANTS

const char BrokerIdentity::CLASS_NAME[] = "BrokerIdentity";

const bdlat_AttributeInfo BrokerIdentity::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_HOST_NAME,
     "hostName",
     sizeof("hostName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_HOST_TAGS,
     "hostTags",
     sizeof("hostTags") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_BROKER_VERSION,
     "brokerVersion",
     sizeof("brokerVersion") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
BrokerIdentity::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            BrokerIdentity::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* BrokerIdentity::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_HOST_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_NAME];
    case ATTRIBUTE_ID_HOST_TAGS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_TAGS];
    case ATTRIBUTE_ID_BROKER_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION];
    default: return 0;
    }
}

// CREATORS

BrokerIdentity::BrokerIdentity(bslma::Allocator* basicAllocator)
: d_hostName(basicAllocator)
, d_hostTags(basicAllocator)
, d_brokerVersion(basicAllocator)
{
}

BrokerIdentity::BrokerIdentity(const BrokerIdentity& original,
                               bslma::Allocator*     basicAllocator)
: d_hostName(original.d_hostName, basicAllocator)
, d_hostTags(original.d_hostTags, basicAllocator)
, d_brokerVersion(original.d_brokerVersion, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerIdentity::BrokerIdentity(BrokerIdentity&& original) noexcept
: d_hostName(bsl::move(original.d_hostName)),
  d_hostTags(bsl::move(original.d_hostTags)),
  d_brokerVersion(bsl::move(original.d_brokerVersion))
{
}

BrokerIdentity::BrokerIdentity(BrokerIdentity&&  original,
                               bslma::Allocator* basicAllocator)
: d_hostName(bsl::move(original.d_hostName), basicAllocator)
, d_hostTags(bsl::move(original.d_hostTags), basicAllocator)
, d_brokerVersion(bsl::move(original.d_brokerVersion), basicAllocator)
{
}
#endif

BrokerIdentity::~BrokerIdentity()
{
}

// MANIPULATORS

BrokerIdentity& BrokerIdentity::operator=(const BrokerIdentity& rhs)
{
    if (this != &rhs) {
        d_hostName      = rhs.d_hostName;
        d_hostTags      = rhs.d_hostTags;
        d_brokerVersion = rhs.d_brokerVersion;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
BrokerIdentity& BrokerIdentity::operator=(BrokerIdentity&& rhs)
{
    if (this != &rhs) {
        d_hostName      = bsl::move(rhs.d_hostName);
        d_hostTags      = bsl::move(rhs.d_hostTags);
        d_brokerVersion = bsl::move(rhs.d_brokerVersion);
    }

    return *this;
}
#endif

void BrokerIdentity::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_hostName);
    bdlat_ValueTypeFunctions::reset(&d_hostTags);
    bdlat_ValueTypeFunctions::reset(&d_brokerVersion);
}

// ACCESSORS

bsl::ostream& BrokerIdentity::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("hostName", this->hostName());
    printer.printAttribute("hostTags", this->hostTags());
    printer.printAttribute("brokerVersion", this->brokerVersion());
    printer.end();
    return stream;
}

// ---------------------
// class DomainConfigRaw
// ---------------------

// CONSTANTS

const char DomainConfigRaw::CLASS_NAME[] = "DomainConfigRaw";

const bdlat_AttributeInfo DomainConfigRaw::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_DOMAIN_NAME,
     "domainName",
     sizeof("domainName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CONFIG,
     "config",
     sizeof("config") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
DomainConfigRaw::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DomainConfigRaw::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* DomainConfigRaw::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_DOMAIN_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_NAME];
    case ATTRIBUTE_ID_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG];
    default: return 0;
    }
}

// CREATORS

DomainConfigRaw::DomainConfigRaw(bslma::Allocator* basicAllocator)
: d_domainName(basicAllocator)
, d_config(basicAllocator)
{
}

DomainConfigRaw::DomainConfigRaw(const DomainConfigRaw& original,
                                 bslma::Allocator*      basicAllocator)
: d_domainName(original.d_domainName, basicAllocator)
, d_config(original.d_config, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainConfigRaw::DomainConfigRaw(DomainConfigRaw&& original) noexcept
: d_domainName(bsl::move(original.d_domainName)),
  d_config(bsl::move(original.d_config))
{
}

DomainConfigRaw::DomainConfigRaw(DomainConfigRaw&& original,
                                 bslma::Allocator* basicAllocator)
: d_domainName(bsl::move(original.d_domainName), basicAllocator)
, d_config(bsl::move(original.d_config), basicAllocator)
{
}
#endif

DomainConfigRaw::~DomainConfigRaw()
{
}

// MANIPULATORS

DomainConfigRaw& DomainConfigRaw::operator=(const DomainConfigRaw& rhs)
{
    if (this != &rhs) {
        d_domainName = rhs.d_domainName;
        d_config     = rhs.d_config;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainConfigRaw& DomainConfigRaw::operator=(DomainConfigRaw&& rhs)
{
    if (this != &rhs) {
        d_domainName = bsl::move(rhs.d_domainName);
        d_config     = bsl::move(rhs.d_config);
    }

    return *this;
}
#endif

void DomainConfigRaw::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_domainName);
    bdlat_ValueTypeFunctions::reset(&d_config);
}

// ACCESSORS

bsl::ostream& DomainConfigRaw::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("domainName", this->domainName());
    printer.printAttribute("config", this->config());
    printer.end();
    return stream;
}

// --------------------
// class DomainResolver
// --------------------

// CONSTANTS

const char DomainResolver::CLASS_NAME[] = "DomainResolver";

const bdlat_AttributeInfo DomainResolver::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CLUSTER,
     "cluster",
     sizeof("cluster") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
DomainResolver::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DomainResolver::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* DomainResolver::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_CLUSTER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER];
    default: return 0;
    }
}

// CREATORS

DomainResolver::DomainResolver(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_cluster(basicAllocator)
{
}

DomainResolver::DomainResolver(const DomainResolver& original,
                               bslma::Allocator*     basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_cluster(original.d_cluster, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainResolver::DomainResolver(DomainResolver&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_cluster(bsl::move(original.d_cluster))
{
}

DomainResolver::DomainResolver(DomainResolver&&  original,
                               bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_cluster(bsl::move(original.d_cluster), basicAllocator)
{
}
#endif

DomainResolver::~DomainResolver()
{
}

// MANIPULATORS

DomainResolver& DomainResolver::operator=(const DomainResolver& rhs)
{
    if (this != &rhs) {
        d_name    = rhs.d_name;
        d_cluster = rhs.d_cluster;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainResolver& DomainResolver::operator=(DomainResolver&& rhs)
{
    if (this != &rhs) {
        d_name    = bsl::move(rhs.d_name);
        d_cluster = bsl::move(rhs.d_cluster);
    }

    return *this;
}
#endif

void DomainResolver::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_cluster);
}

// ACCESSORS

bsl::ostream& DomainResolver::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("cluster", this->cluster());
    printer.end();
    return stream;
}

// -----------------------
// class ExpressionVersion
// -----------------------

// CONSTANTS

const char ExpressionVersion::CLASS_NAME[] = "ExpressionVersion";

const bdlat_EnumeratorInfo ExpressionVersion::ENUMERATOR_INFO_ARRAY[] = {
    {ExpressionVersion::E_UNDEFINED,
     "E_UNDEFINED",
     sizeof("E_UNDEFINED") - 1,
     ""},
    {ExpressionVersion::E_VERSION_1,
     "E_VERSION_1",
     sizeof("E_VERSION_1") - 1,
     ""}};

// CLASS METHODS

int ExpressionVersion::fromInt(ExpressionVersion::Value* result, int number)
{
    switch (number) {
    case ExpressionVersion::E_UNDEFINED:
    case ExpressionVersion::E_VERSION_1:
        *result = static_cast<ExpressionVersion::Value>(number);
        return 0;
    default: return -1;
    }
}

int ExpressionVersion::fromString(ExpressionVersion::Value* result,
                                  const char*               string,
                                  int                       stringLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            ExpressionVersion::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<ExpressionVersion::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* ExpressionVersion::toString(ExpressionVersion::Value value)
{
    switch (value) {
    case E_UNDEFINED: {
        return "E_UNDEFINED";
    }
    case E_VERSION_1: {
        return "E_VERSION_1";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -------------
// class Failure
// -------------

// CONSTANTS

const char Failure::CLASS_NAME[] = "Failure";

const char Failure::DEFAULT_INITIALIZER_MESSAGE[] = "";

const bdlat_AttributeInfo Failure::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CODE,
     "code",
     sizeof("code") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MESSAGE,
     "message",
     sizeof("message") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Failure::lookupAttributeInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Failure::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Failure::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CODE: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CODE];
    case ATTRIBUTE_ID_MESSAGE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE];
    default: return 0;
    }
}

// CREATORS

Failure::Failure(bslma::Allocator* basicAllocator)
: d_message(DEFAULT_INITIALIZER_MESSAGE, basicAllocator)
, d_code()
{
}

Failure::Failure(const Failure& original, bslma::Allocator* basicAllocator)
: d_message(original.d_message, basicAllocator)
, d_code(original.d_code)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Failure::Failure(Failure&& original) noexcept
: d_message(bsl::move(original.d_message)),
  d_code(bsl::move(original.d_code))
{
}

Failure::Failure(Failure&& original, bslma::Allocator* basicAllocator)
: d_message(bsl::move(original.d_message), basicAllocator)
, d_code(bsl::move(original.d_code))
{
}
#endif

Failure::~Failure()
{
}

// MANIPULATORS

Failure& Failure::operator=(const Failure& rhs)
{
    if (this != &rhs) {
        d_code    = rhs.d_code;
        d_message = rhs.d_message;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Failure& Failure::operator=(Failure&& rhs)
{
    if (this != &rhs) {
        d_code    = bsl::move(rhs.d_code);
        d_message = bsl::move(rhs.d_message);
    }

    return *this;
}
#endif

void Failure::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_code);
    d_message = DEFAULT_INITIALIZER_MESSAGE;
}

// ACCESSORS

bsl::ostream&
Failure::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("code", this->code());
    printer.printAttribute("message", this->message());
    printer.end();
    return stream;
}

// -----------------------
// class FileBackedStorage
// -----------------------

// CONSTANTS

const char FileBackedStorage::CLASS_NAME[] = "FileBackedStorage";

// CLASS METHODS

const bdlat_AttributeInfo*
FileBackedStorage::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* FileBackedStorage::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

FileBackedStorage::FileBackedStorage()
{
}

FileBackedStorage::FileBackedStorage(const FileBackedStorage& original)
{
    (void)original;
}

FileBackedStorage::~FileBackedStorage()
{
}

// MANIPULATORS

FileBackedStorage& FileBackedStorage::operator=(const FileBackedStorage& rhs)
{
    (void)rhs;
    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FileBackedStorage& FileBackedStorage::operator=(FileBackedStorage&& rhs)
{
    (void)rhs;
    return *this;
}
#endif

void FileBackedStorage::reset()
{
}

// ACCESSORS

bsl::ostream& FileBackedStorage::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    (void)level;
    (void)spacesPerLevel;
    return stream;
}

// ---------------------
// class InMemoryStorage
// ---------------------

// CONSTANTS

const char InMemoryStorage::CLASS_NAME[] = "InMemoryStorage";

// CLASS METHODS

const bdlat_AttributeInfo*
InMemoryStorage::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* InMemoryStorage::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

InMemoryStorage::InMemoryStorage()
{
}

InMemoryStorage::InMemoryStorage(const InMemoryStorage& original)
{
    (void)original;
}

InMemoryStorage::~InMemoryStorage()
{
}

// MANIPULATORS

InMemoryStorage& InMemoryStorage::operator=(const InMemoryStorage& rhs)
{
    (void)rhs;
    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
InMemoryStorage& InMemoryStorage::operator=(InMemoryStorage&& rhs)
{
    (void)rhs;
    return *this;
}
#endif

void InMemoryStorage::reset()
{
}

// ACCESSORS

bsl::ostream& InMemoryStorage::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    (void)level;
    (void)spacesPerLevel;
    return stream;
}

// ------------
// class Limits
// ------------

// CONSTANTS

const char Limits::CLASS_NAME[] = "Limits";

const double Limits::DEFAULT_INITIALIZER_MESSAGES_WATERMARK_RATIO = 0.8;

const double Limits::DEFAULT_INITIALIZER_BYTES_WATERMARK_RATIO = 0.8;

const bdlat_AttributeInfo Limits::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_MESSAGES,
     "messages",
     sizeof("messages") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MESSAGES_WATERMARK_RATIO,
     "messagesWatermarkRatio",
     sizeof("messagesWatermarkRatio") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BYTES,
     "bytes",
     sizeof("bytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_BYTES_WATERMARK_RATIO,
     "bytesWatermarkRatio",
     sizeof("bytesWatermarkRatio") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* Limits::lookupAttributeInfo(const char* name,
                                                       int         nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Limits::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Limits::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_MESSAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES];
    case ATTRIBUTE_ID_MESSAGES_WATERMARK_RATIO:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGES_WATERMARK_RATIO];
    case ATTRIBUTE_ID_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES];
    case ATTRIBUTE_ID_BYTES_WATERMARK_RATIO:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BYTES_WATERMARK_RATIO];
    default: return 0;
    }
}

// CREATORS

Limits::Limits()
: d_messagesWatermarkRatio(DEFAULT_INITIALIZER_MESSAGES_WATERMARK_RATIO)
, d_bytesWatermarkRatio(DEFAULT_INITIALIZER_BYTES_WATERMARK_RATIO)
, d_messages()
, d_bytes()
{
}

Limits::Limits(const Limits& original)
: d_messagesWatermarkRatio(original.d_messagesWatermarkRatio)
, d_bytesWatermarkRatio(original.d_bytesWatermarkRatio)
, d_messages(original.d_messages)
, d_bytes(original.d_bytes)
{
}

Limits::~Limits()
{
}

// MANIPULATORS

Limits& Limits::operator=(const Limits& rhs)
{
    if (this != &rhs) {
        d_messages               = rhs.d_messages;
        d_messagesWatermarkRatio = rhs.d_messagesWatermarkRatio;
        d_bytes                  = rhs.d_bytes;
        d_bytesWatermarkRatio    = rhs.d_bytesWatermarkRatio;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Limits& Limits::operator=(Limits&& rhs)
{
    if (this != &rhs) {
        d_messages               = bsl::move(rhs.d_messages);
        d_messagesWatermarkRatio = bsl::move(rhs.d_messagesWatermarkRatio);
        d_bytes                  = bsl::move(rhs.d_bytes);
        d_bytesWatermarkRatio    = bsl::move(rhs.d_bytesWatermarkRatio);
    }

    return *this;
}
#endif

void Limits::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_messages);
    d_messagesWatermarkRatio = DEFAULT_INITIALIZER_MESSAGES_WATERMARK_RATIO;
    bdlat_ValueTypeFunctions::reset(&d_bytes);
    d_bytesWatermarkRatio = DEFAULT_INITIALIZER_BYTES_WATERMARK_RATIO;
}

// ACCESSORS

bsl::ostream&
Limits::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("messages", this->messages());
    printer.printAttribute("messagesWatermarkRatio",
                           this->messagesWatermarkRatio());
    printer.printAttribute("bytes", this->bytes());
    printer.printAttribute("bytesWatermarkRatio", this->bytesWatermarkRatio());
    printer.end();
    return stream;
}

// ----------------------
// class MsgGroupIdConfig
// ----------------------

// CONSTANTS

const char MsgGroupIdConfig::CLASS_NAME[] = "MsgGroupIdConfig";

const bool MsgGroupIdConfig::DEFAULT_INITIALIZER_REBALANCE = false;

const int MsgGroupIdConfig::DEFAULT_INITIALIZER_MAX_GROUPS = 2147483647;

const bsls::Types::Int64 MsgGroupIdConfig::DEFAULT_INITIALIZER_TTL_SECONDS = 0;

const bdlat_AttributeInfo MsgGroupIdConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_REBALANCE,
     "rebalance",
     sizeof("rebalance") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_MAX_GROUPS,
     "maxGroups",
     sizeof("maxGroups") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_TTL_SECONDS,
     "ttlSeconds",
     sizeof("ttlSeconds") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
MsgGroupIdConfig::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            MsgGroupIdConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* MsgGroupIdConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_REBALANCE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REBALANCE];
    case ATTRIBUTE_ID_MAX_GROUPS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_GROUPS];
    case ATTRIBUTE_ID_TTL_SECONDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TTL_SECONDS];
    default: return 0;
    }
}

// CREATORS

MsgGroupIdConfig::MsgGroupIdConfig()
: d_ttlSeconds(DEFAULT_INITIALIZER_TTL_SECONDS)
, d_maxGroups(DEFAULT_INITIALIZER_MAX_GROUPS)
, d_rebalance(DEFAULT_INITIALIZER_REBALANCE)
{
}

MsgGroupIdConfig::MsgGroupIdConfig(const MsgGroupIdConfig& original)
: d_ttlSeconds(original.d_ttlSeconds)
, d_maxGroups(original.d_maxGroups)
, d_rebalance(original.d_rebalance)
{
}

MsgGroupIdConfig::~MsgGroupIdConfig()
{
}

// MANIPULATORS

MsgGroupIdConfig& MsgGroupIdConfig::operator=(const MsgGroupIdConfig& rhs)
{
    if (this != &rhs) {
        d_rebalance  = rhs.d_rebalance;
        d_maxGroups  = rhs.d_maxGroups;
        d_ttlSeconds = rhs.d_ttlSeconds;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
MsgGroupIdConfig& MsgGroupIdConfig::operator=(MsgGroupIdConfig&& rhs)
{
    if (this != &rhs) {
        d_rebalance  = bsl::move(rhs.d_rebalance);
        d_maxGroups  = bsl::move(rhs.d_maxGroups);
        d_ttlSeconds = bsl::move(rhs.d_ttlSeconds);
    }

    return *this;
}
#endif

void MsgGroupIdConfig::reset()
{
    d_rebalance  = DEFAULT_INITIALIZER_REBALANCE;
    d_maxGroups  = DEFAULT_INITIALIZER_MAX_GROUPS;
    d_ttlSeconds = DEFAULT_INITIALIZER_TTL_SECONDS;
}

// ACCESSORS

bsl::ostream& MsgGroupIdConfig::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("rebalance", this->rebalance());
    printer.printAttribute("maxGroups", this->maxGroups());
    printer.printAttribute("ttlSeconds", this->ttlSeconds());
    printer.end();
    return stream;
}

// ------------------------------
// class QueueConsistencyEventual
// ------------------------------

// CONSTANTS

const char QueueConsistencyEventual::CLASS_NAME[] = "QueueConsistencyEventual";

// CLASS METHODS

const bdlat_AttributeInfo*
QueueConsistencyEventual::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo*
QueueConsistencyEventual::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

QueueConsistencyEventual::QueueConsistencyEventual()
{
}

QueueConsistencyEventual::QueueConsistencyEventual(
    const QueueConsistencyEventual& original)
{
    (void)original;
}

QueueConsistencyEventual::~QueueConsistencyEventual()
{
}

// MANIPULATORS

QueueConsistencyEventual&
QueueConsistencyEventual::operator=(const QueueConsistencyEventual& rhs)
{
    (void)rhs;
    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueConsistencyEventual&
QueueConsistencyEventual::operator=(QueueConsistencyEventual&& rhs)
{
    (void)rhs;
    return *this;
}
#endif

void QueueConsistencyEventual::reset()
{
}

// ACCESSORS

bsl::ostream& QueueConsistencyEventual::print(bsl::ostream& stream,
                                              int           level,
                                              int spacesPerLevel) const
{
    (void)level;
    (void)spacesPerLevel;
    return stream;
}

// ----------------------------
// class QueueConsistencyStrong
// ----------------------------

// CONSTANTS

const char QueueConsistencyStrong::CLASS_NAME[] = "QueueConsistencyStrong";

// CLASS METHODS

const bdlat_AttributeInfo*
QueueConsistencyStrong::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* QueueConsistencyStrong::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

QueueConsistencyStrong::QueueConsistencyStrong()
{
}

QueueConsistencyStrong::QueueConsistencyStrong(
    const QueueConsistencyStrong& original)
{
    (void)original;
}

QueueConsistencyStrong::~QueueConsistencyStrong()
{
}

// MANIPULATORS

QueueConsistencyStrong&
QueueConsistencyStrong::operator=(const QueueConsistencyStrong& rhs)
{
    (void)rhs;
    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueConsistencyStrong&
QueueConsistencyStrong::operator=(QueueConsistencyStrong&& rhs)
{
    (void)rhs;
    return *this;
}
#endif

void QueueConsistencyStrong::reset()
{
}

// ACCESSORS

bsl::ostream& QueueConsistencyStrong::print(bsl::ostream& stream,
                                            int           level,
                                            int           spacesPerLevel) const
{
    (void)level;
    (void)spacesPerLevel;
    return stream;
}

// ------------------------
// class QueueModeBroadcast
// ------------------------

// CONSTANTS

const char QueueModeBroadcast::CLASS_NAME[] = "QueueModeBroadcast";

// CLASS METHODS

const bdlat_AttributeInfo*
QueueModeBroadcast::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* QueueModeBroadcast::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

QueueModeBroadcast::QueueModeBroadcast()
{
}

QueueModeBroadcast::QueueModeBroadcast(const QueueModeBroadcast& original)
{
    (void)original;
}

QueueModeBroadcast::~QueueModeBroadcast()
{
}

// MANIPULATORS

QueueModeBroadcast&
QueueModeBroadcast::operator=(const QueueModeBroadcast& rhs)
{
    (void)rhs;
    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueModeBroadcast& QueueModeBroadcast::operator=(QueueModeBroadcast&& rhs)
{
    (void)rhs;
    return *this;
}
#endif

void QueueModeBroadcast::reset()
{
}

// ACCESSORS

bsl::ostream& QueueModeBroadcast::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    (void)level;
    (void)spacesPerLevel;
    return stream;
}

// ---------------------
// class QueueModeFanout
// ---------------------

// CONSTANTS

const char QueueModeFanout::CLASS_NAME[] = "QueueModeFanout";

const bdlat_AttributeInfo QueueModeFanout::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_APP_I_DS,
     "appIDs",
     sizeof("appIDs") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueModeFanout::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueModeFanout::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueModeFanout::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_APP_I_DS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_I_DS];
    default: return 0;
    }
}

// CREATORS

QueueModeFanout::QueueModeFanout(bslma::Allocator* basicAllocator)
: d_appIDs(basicAllocator)
{
}

QueueModeFanout::QueueModeFanout(const QueueModeFanout& original,
                                 bslma::Allocator*      basicAllocator)
: d_appIDs(original.d_appIDs, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueModeFanout::QueueModeFanout(QueueModeFanout&& original) noexcept
: d_appIDs(bsl::move(original.d_appIDs))
{
}

QueueModeFanout::QueueModeFanout(QueueModeFanout&& original,
                                 bslma::Allocator* basicAllocator)
: d_appIDs(bsl::move(original.d_appIDs), basicAllocator)
{
}
#endif

QueueModeFanout::~QueueModeFanout()
{
}

// MANIPULATORS

QueueModeFanout& QueueModeFanout::operator=(const QueueModeFanout& rhs)
{
    if (this != &rhs) {
        d_appIDs = rhs.d_appIDs;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueModeFanout& QueueModeFanout::operator=(QueueModeFanout&& rhs)
{
    if (this != &rhs) {
        d_appIDs = bsl::move(rhs.d_appIDs);
    }

    return *this;
}
#endif

void QueueModeFanout::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_appIDs);
}

// ACCESSORS

bsl::ostream& QueueModeFanout::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("appIDs", this->appIDs());
    printer.end();
    return stream;
}

// -----------------------
// class QueueModePriority
// -----------------------

// CONSTANTS

const char QueueModePriority::CLASS_NAME[] = "QueueModePriority";

// CLASS METHODS

const bdlat_AttributeInfo*
QueueModePriority::lookupAttributeInfo(const char* name, int nameLength)
{
    (void)name;
    (void)nameLength;
    return 0;
}

const bdlat_AttributeInfo* QueueModePriority::lookupAttributeInfo(int id)
{
    switch (id) {
    default: return 0;
    }
}

// CREATORS

QueueModePriority::QueueModePriority()
{
}

QueueModePriority::QueueModePriority(const QueueModePriority& original)
{
    (void)original;
}

QueueModePriority::~QueueModePriority()
{
}

// MANIPULATORS

QueueModePriority& QueueModePriority::operator=(const QueueModePriority& rhs)
{
    (void)rhs;
    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueModePriority& QueueModePriority::operator=(QueueModePriority&& rhs)
{
    (void)rhs;
    return *this;
}
#endif

void QueueModePriority::reset()
{
}

// ACCESSORS

bsl::ostream& QueueModePriority::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    (void)level;
    (void)spacesPerLevel;
    return stream;
}

// -----------------
// class Consistency
// -----------------

// CONSTANTS

const char Consistency::CLASS_NAME[] = "Consistency";

const bdlat_SelectionInfo Consistency::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_EVENTUAL,
     "eventual",
     sizeof("eventual") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_STRONG,
     "strong",
     sizeof("strong") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* Consistency::lookupSelectionInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            Consistency::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* Consistency::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_EVENTUAL:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_EVENTUAL];
    case SELECTION_ID_STRONG:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_STRONG];
    default: return 0;
    }
}

// CREATORS

Consistency::Consistency(const Consistency& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_EVENTUAL: {
        new (d_eventual.buffer())
            QueueConsistencyEventual(original.d_eventual.object());
    } break;
    case SELECTION_ID_STRONG: {
        new (d_strong.buffer())
            QueueConsistencyStrong(original.d_strong.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Consistency::Consistency(Consistency&& original) noexcept
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_EVENTUAL: {
        new (d_eventual.buffer())
            QueueConsistencyEventual(bsl::move(original.d_eventual.object()));
    } break;
    case SELECTION_ID_STRONG: {
        new (d_strong.buffer())
            QueueConsistencyStrong(bsl::move(original.d_strong.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

Consistency& Consistency::operator=(const Consistency& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_EVENTUAL: {
            makeEventual(rhs.d_eventual.object());
        } break;
        case SELECTION_ID_STRONG: {
            makeStrong(rhs.d_strong.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Consistency& Consistency::operator=(Consistency&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_EVENTUAL: {
            makeEventual(bsl::move(rhs.d_eventual.object()));
        } break;
        case SELECTION_ID_STRONG: {
            makeStrong(bsl::move(rhs.d_strong.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void Consistency::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_EVENTUAL: {
        d_eventual.object().~QueueConsistencyEventual();
    } break;
    case SELECTION_ID_STRONG: {
        d_strong.object().~QueueConsistencyStrong();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int Consistency::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_EVENTUAL: {
        makeEventual();
    } break;
    case SELECTION_ID_STRONG: {
        makeStrong();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int Consistency::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

QueueConsistencyEventual& Consistency::makeEventual()
{
    if (SELECTION_ID_EVENTUAL == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_eventual.object());
    }
    else {
        reset();
        new (d_eventual.buffer()) QueueConsistencyEventual();
        d_selectionId = SELECTION_ID_EVENTUAL;
    }

    return d_eventual.object();
}

QueueConsistencyEventual&
Consistency::makeEventual(const QueueConsistencyEventual& value)
{
    if (SELECTION_ID_EVENTUAL == d_selectionId) {
        d_eventual.object() = value;
    }
    else {
        reset();
        new (d_eventual.buffer()) QueueConsistencyEventual(value);
        d_selectionId = SELECTION_ID_EVENTUAL;
    }

    return d_eventual.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueConsistencyEventual&
Consistency::makeEventual(QueueConsistencyEventual&& value)
{
    if (SELECTION_ID_EVENTUAL == d_selectionId) {
        d_eventual.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_eventual.buffer()) QueueConsistencyEventual(bsl::move(value));
        d_selectionId = SELECTION_ID_EVENTUAL;
    }

    return d_eventual.object();
}
#endif

QueueConsistencyStrong& Consistency::makeStrong()
{
    if (SELECTION_ID_STRONG == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_strong.object());
    }
    else {
        reset();
        new (d_strong.buffer()) QueueConsistencyStrong();
        d_selectionId = SELECTION_ID_STRONG;
    }

    return d_strong.object();
}

QueueConsistencyStrong&
Consistency::makeStrong(const QueueConsistencyStrong& value)
{
    if (SELECTION_ID_STRONG == d_selectionId) {
        d_strong.object() = value;
    }
    else {
        reset();
        new (d_strong.buffer()) QueueConsistencyStrong(value);
        d_selectionId = SELECTION_ID_STRONG;
    }

    return d_strong.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueConsistencyStrong& Consistency::makeStrong(QueueConsistencyStrong&& value)
{
    if (SELECTION_ID_STRONG == d_selectionId) {
        d_strong.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_strong.buffer()) QueueConsistencyStrong(bsl::move(value));
        d_selectionId = SELECTION_ID_STRONG;
    }

    return d_strong.object();
}
#endif

// ACCESSORS

bsl::ostream&
Consistency::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_EVENTUAL: {
        printer.printAttribute("eventual", d_eventual.object());
    } break;
    case SELECTION_ID_STRONG: {
        printer.printAttribute("strong", d_strong.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* Consistency::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_EVENTUAL:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_EVENTUAL].name();
    case SELECTION_ID_STRONG:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_STRONG].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -------------------------
// class DomainConfigRequest
// -------------------------

// CONSTANTS

const char DomainConfigRequest::CLASS_NAME[] = "DomainConfigRequest";

const bdlat_AttributeInfo DomainConfigRequest::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_BROKER_IDENTITY,
     "brokerIdentity",
     sizeof("brokerIdentity") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_DOMAIN_NAME,
     "domainName",
     sizeof("domainName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
DomainConfigRequest::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DomainConfigRequest::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* DomainConfigRequest::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_BROKER_IDENTITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_IDENTITY];
    case ATTRIBUTE_ID_DOMAIN_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_NAME];
    default: return 0;
    }
}

// CREATORS

DomainConfigRequest::DomainConfigRequest(bslma::Allocator* basicAllocator)
: d_domainName(basicAllocator)
, d_brokerIdentity(basicAllocator)
{
}

DomainConfigRequest::DomainConfigRequest(const DomainConfigRequest& original,
                                         bslma::Allocator* basicAllocator)
: d_domainName(original.d_domainName, basicAllocator)
, d_brokerIdentity(original.d_brokerIdentity, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainConfigRequest::DomainConfigRequest(DomainConfigRequest&& original)
    noexcept : d_domainName(bsl::move(original.d_domainName)),
               d_brokerIdentity(bsl::move(original.d_brokerIdentity))
{
}

DomainConfigRequest::DomainConfigRequest(DomainConfigRequest&& original,
                                         bslma::Allocator*     basicAllocator)
: d_domainName(bsl::move(original.d_domainName), basicAllocator)
, d_brokerIdentity(bsl::move(original.d_brokerIdentity), basicAllocator)
{
}
#endif

DomainConfigRequest::~DomainConfigRequest()
{
}

// MANIPULATORS

DomainConfigRequest&
DomainConfigRequest::operator=(const DomainConfigRequest& rhs)
{
    if (this != &rhs) {
        d_brokerIdentity = rhs.d_brokerIdentity;
        d_domainName     = rhs.d_domainName;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainConfigRequest& DomainConfigRequest::operator=(DomainConfigRequest&& rhs)
{
    if (this != &rhs) {
        d_brokerIdentity = bsl::move(rhs.d_brokerIdentity);
        d_domainName     = bsl::move(rhs.d_domainName);
    }

    return *this;
}
#endif

void DomainConfigRequest::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_brokerIdentity);
    bdlat_ValueTypeFunctions::reset(&d_domainName);
}

// ACCESSORS

bsl::ostream& DomainConfigRequest::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("brokerIdentity", this->brokerIdentity());
    printer.printAttribute("domainName", this->domainName());
    printer.end();
    return stream;
}

// ----------------
// class Expression
// ----------------

// CONSTANTS

const char Expression::CLASS_NAME[] = "Expression";

const ExpressionVersion::Value Expression::DEFAULT_INITIALIZER_VERSION =
    ExpressionVersion::E_UNDEFINED;

const bdlat_AttributeInfo Expression::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_VERSION,
     "version",
     sizeof("version") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_TEXT,
     "text",
     sizeof("text") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Expression::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Expression::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Expression::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERSION];
    case ATTRIBUTE_ID_TEXT: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TEXT];
    default: return 0;
    }
}

// CREATORS

Expression::Expression(bslma::Allocator* basicAllocator)
: d_text(basicAllocator)
, d_version(DEFAULT_INITIALIZER_VERSION)
{
}

Expression::Expression(const Expression& original,
                       bslma::Allocator* basicAllocator)
: d_text(original.d_text, basicAllocator)
, d_version(original.d_version)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Expression::Expression(Expression&& original) noexcept
: d_text(bsl::move(original.d_text)),
  d_version(bsl::move(original.d_version))
{
}

Expression::Expression(Expression&& original, bslma::Allocator* basicAllocator)
: d_text(bsl::move(original.d_text), basicAllocator)
, d_version(bsl::move(original.d_version))
{
}
#endif

Expression::~Expression()
{
}

// MANIPULATORS

Expression& Expression::operator=(const Expression& rhs)
{
    if (this != &rhs) {
        d_version = rhs.d_version;
        d_text    = rhs.d_text;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Expression& Expression::operator=(Expression&& rhs)
{
    if (this != &rhs) {
        d_version = bsl::move(rhs.d_version);
        d_text    = bsl::move(rhs.d_text);
    }

    return *this;
}
#endif

void Expression::reset()
{
    d_version = DEFAULT_INITIALIZER_VERSION;
    bdlat_ValueTypeFunctions::reset(&d_text);
}

// ACCESSORS

bsl::ostream&
Expression::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("version", this->version());
    printer.printAttribute("text", this->text());
    printer.end();
    return stream;
}

// ---------------
// class QueueMode
// ---------------

// CONSTANTS

const char QueueMode::CLASS_NAME[] = "QueueMode";

const bdlat_SelectionInfo QueueMode::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_FANOUT,
     "fanout",
     sizeof("fanout") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_PRIORITY,
     "priority",
     sizeof("priority") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_BROADCAST,
     "broadcast",
     sizeof("broadcast") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* QueueMode::lookupSelectionInfo(const char* name,
                                                          int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            QueueMode::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* QueueMode::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_FANOUT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_FANOUT];
    case SELECTION_ID_PRIORITY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_PRIORITY];
    case SELECTION_ID_BROADCAST:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_BROADCAST];
    default: return 0;
    }
}

// CREATORS

QueueMode::QueueMode(const QueueMode&  original,
                     bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT: {
        new (d_fanout.buffer())
            QueueModeFanout(original.d_fanout.object(), d_allocator_p);
    } break;
    case SELECTION_ID_PRIORITY: {
        new (d_priority.buffer())
            QueueModePriority(original.d_priority.object());
    } break;
    case SELECTION_ID_BROADCAST: {
        new (d_broadcast.buffer())
            QueueModeBroadcast(original.d_broadcast.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueMode::QueueMode(QueueMode&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT: {
        new (d_fanout.buffer())
            QueueModeFanout(bsl::move(original.d_fanout.object()),
                            d_allocator_p);
    } break;
    case SELECTION_ID_PRIORITY: {
        new (d_priority.buffer())
            QueueModePriority(bsl::move(original.d_priority.object()));
    } break;
    case SELECTION_ID_BROADCAST: {
        new (d_broadcast.buffer())
            QueueModeBroadcast(bsl::move(original.d_broadcast.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

QueueMode::QueueMode(QueueMode&& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT: {
        new (d_fanout.buffer())
            QueueModeFanout(bsl::move(original.d_fanout.object()),
                            d_allocator_p);
    } break;
    case SELECTION_ID_PRIORITY: {
        new (d_priority.buffer())
            QueueModePriority(bsl::move(original.d_priority.object()));
    } break;
    case SELECTION_ID_BROADCAST: {
        new (d_broadcast.buffer())
            QueueModeBroadcast(bsl::move(original.d_broadcast.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

QueueMode& QueueMode::operator=(const QueueMode& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_FANOUT: {
            makeFanout(rhs.d_fanout.object());
        } break;
        case SELECTION_ID_PRIORITY: {
            makePriority(rhs.d_priority.object());
        } break;
        case SELECTION_ID_BROADCAST: {
            makeBroadcast(rhs.d_broadcast.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueMode& QueueMode::operator=(QueueMode&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_FANOUT: {
            makeFanout(bsl::move(rhs.d_fanout.object()));
        } break;
        case SELECTION_ID_PRIORITY: {
            makePriority(bsl::move(rhs.d_priority.object()));
        } break;
        case SELECTION_ID_BROADCAST: {
            makeBroadcast(bsl::move(rhs.d_broadcast.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void QueueMode::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT: {
        d_fanout.object().~QueueModeFanout();
    } break;
    case SELECTION_ID_PRIORITY: {
        d_priority.object().~QueueModePriority();
    } break;
    case SELECTION_ID_BROADCAST: {
        d_broadcast.object().~QueueModeBroadcast();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int QueueMode::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_FANOUT: {
        makeFanout();
    } break;
    case SELECTION_ID_PRIORITY: {
        makePriority();
    } break;
    case SELECTION_ID_BROADCAST: {
        makeBroadcast();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int QueueMode::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

QueueModeFanout& QueueMode::makeFanout()
{
    if (SELECTION_ID_FANOUT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_fanout.object());
    }
    else {
        reset();
        new (d_fanout.buffer()) QueueModeFanout(d_allocator_p);
        d_selectionId = SELECTION_ID_FANOUT;
    }

    return d_fanout.object();
}

QueueModeFanout& QueueMode::makeFanout(const QueueModeFanout& value)
{
    if (SELECTION_ID_FANOUT == d_selectionId) {
        d_fanout.object() = value;
    }
    else {
        reset();
        new (d_fanout.buffer()) QueueModeFanout(value, d_allocator_p);
        d_selectionId = SELECTION_ID_FANOUT;
    }

    return d_fanout.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueModeFanout& QueueMode::makeFanout(QueueModeFanout&& value)
{
    if (SELECTION_ID_FANOUT == d_selectionId) {
        d_fanout.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_fanout.buffer())
            QueueModeFanout(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_FANOUT;
    }

    return d_fanout.object();
}
#endif

QueueModePriority& QueueMode::makePriority()
{
    if (SELECTION_ID_PRIORITY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_priority.object());
    }
    else {
        reset();
        new (d_priority.buffer()) QueueModePriority();
        d_selectionId = SELECTION_ID_PRIORITY;
    }

    return d_priority.object();
}

QueueModePriority& QueueMode::makePriority(const QueueModePriority& value)
{
    if (SELECTION_ID_PRIORITY == d_selectionId) {
        d_priority.object() = value;
    }
    else {
        reset();
        new (d_priority.buffer()) QueueModePriority(value);
        d_selectionId = SELECTION_ID_PRIORITY;
    }

    return d_priority.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueModePriority& QueueMode::makePriority(QueueModePriority&& value)
{
    if (SELECTION_ID_PRIORITY == d_selectionId) {
        d_priority.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_priority.buffer()) QueueModePriority(bsl::move(value));
        d_selectionId = SELECTION_ID_PRIORITY;
    }

    return d_priority.object();
}
#endif

QueueModeBroadcast& QueueMode::makeBroadcast()
{
    if (SELECTION_ID_BROADCAST == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_broadcast.object());
    }
    else {
        reset();
        new (d_broadcast.buffer()) QueueModeBroadcast();
        d_selectionId = SELECTION_ID_BROADCAST;
    }

    return d_broadcast.object();
}

QueueModeBroadcast& QueueMode::makeBroadcast(const QueueModeBroadcast& value)
{
    if (SELECTION_ID_BROADCAST == d_selectionId) {
        d_broadcast.object() = value;
    }
    else {
        reset();
        new (d_broadcast.buffer()) QueueModeBroadcast(value);
        d_selectionId = SELECTION_ID_BROADCAST;
    }

    return d_broadcast.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
QueueModeBroadcast& QueueMode::makeBroadcast(QueueModeBroadcast&& value)
{
    if (SELECTION_ID_BROADCAST == d_selectionId) {
        d_broadcast.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_broadcast.buffer()) QueueModeBroadcast(bsl::move(value));
        d_selectionId = SELECTION_ID_BROADCAST;
    }

    return d_broadcast.object();
}
#endif

// ACCESSORS

bsl::ostream&
QueueMode::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT: {
        printer.printAttribute("fanout", d_fanout.object());
    } break;
    case SELECTION_ID_PRIORITY: {
        printer.printAttribute("priority", d_priority.object());
    } break;
    case SELECTION_ID_BROADCAST: {
        printer.printAttribute("broadcast", d_broadcast.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* QueueMode::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_FANOUT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_FANOUT].name();
    case SELECTION_ID_PRIORITY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_PRIORITY].name();
    case SELECTION_ID_BROADCAST:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_BROADCAST].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// --------------
// class Response
// --------------

// CONSTANTS

const char Response::CLASS_NAME[] = "Response";

const bdlat_SelectionInfo Response::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_FAILURE,
     "failure",
     sizeof("failure") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_DOMAIN_CONFIG,
     "domainConfig",
     sizeof("domainConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* Response::lookupSelectionInfo(const char* name,
                                                         int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            Response::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* Response::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_FAILURE:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_FAILURE];
    case SELECTION_ID_DOMAIN_CONFIG:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_CONFIG];
    default: return 0;
    }
}

// CREATORS

Response::Response(const Response& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_FAILURE: {
        new (d_failure.buffer())
            Failure(original.d_failure.object(), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_CONFIG: {
        new (d_domainConfig.buffer())
            DomainConfigRaw(original.d_domainConfig.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Response::Response(Response&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_FAILURE: {
        new (d_failure.buffer())
            Failure(bsl::move(original.d_failure.object()), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_CONFIG: {
        new (d_domainConfig.buffer())
            DomainConfigRaw(bsl::move(original.d_domainConfig.object()),
                            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

Response::Response(Response&& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_FAILURE: {
        new (d_failure.buffer())
            Failure(bsl::move(original.d_failure.object()), d_allocator_p);
    } break;
    case SELECTION_ID_DOMAIN_CONFIG: {
        new (d_domainConfig.buffer())
            DomainConfigRaw(bsl::move(original.d_domainConfig.object()),
                            d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

Response& Response::operator=(const Response& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_FAILURE: {
            makeFailure(rhs.d_failure.object());
        } break;
        case SELECTION_ID_DOMAIN_CONFIG: {
            makeDomainConfig(rhs.d_domainConfig.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Response& Response::operator=(Response&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_FAILURE: {
            makeFailure(bsl::move(rhs.d_failure.object()));
        } break;
        case SELECTION_ID_DOMAIN_CONFIG: {
            makeDomainConfig(bsl::move(rhs.d_domainConfig.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void Response::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_FAILURE: {
        d_failure.object().~Failure();
    } break;
    case SELECTION_ID_DOMAIN_CONFIG: {
        d_domainConfig.object().~DomainConfigRaw();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int Response::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_FAILURE: {
        makeFailure();
    } break;
    case SELECTION_ID_DOMAIN_CONFIG: {
        makeDomainConfig();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int Response::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

Failure& Response::makeFailure()
{
    if (SELECTION_ID_FAILURE == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_failure.object());
    }
    else {
        reset();
        new (d_failure.buffer()) Failure(d_allocator_p);
        d_selectionId = SELECTION_ID_FAILURE;
    }

    return d_failure.object();
}

Failure& Response::makeFailure(const Failure& value)
{
    if (SELECTION_ID_FAILURE == d_selectionId) {
        d_failure.object() = value;
    }
    else {
        reset();
        new (d_failure.buffer()) Failure(value, d_allocator_p);
        d_selectionId = SELECTION_ID_FAILURE;
    }

    return d_failure.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Failure& Response::makeFailure(Failure&& value)
{
    if (SELECTION_ID_FAILURE == d_selectionId) {
        d_failure.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_failure.buffer()) Failure(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_FAILURE;
    }

    return d_failure.object();
}
#endif

DomainConfigRaw& Response::makeDomainConfig()
{
    if (SELECTION_ID_DOMAIN_CONFIG == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domainConfig.object());
    }
    else {
        reset();
        new (d_domainConfig.buffer()) DomainConfigRaw(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_CONFIG;
    }

    return d_domainConfig.object();
}

DomainConfigRaw& Response::makeDomainConfig(const DomainConfigRaw& value)
{
    if (SELECTION_ID_DOMAIN_CONFIG == d_selectionId) {
        d_domainConfig.object() = value;
    }
    else {
        reset();
        new (d_domainConfig.buffer()) DomainConfigRaw(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_CONFIG;
    }

    return d_domainConfig.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainConfigRaw& Response::makeDomainConfig(DomainConfigRaw&& value)
{
    if (SELECTION_ID_DOMAIN_CONFIG == d_selectionId) {
        d_domainConfig.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domainConfig.buffer())
            DomainConfigRaw(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_CONFIG;
    }

    return d_domainConfig.object();
}
#endif

// ACCESSORS

bsl::ostream&
Response::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_FAILURE: {
        printer.printAttribute("failure", d_failure.object());
    } break;
    case SELECTION_ID_DOMAIN_CONFIG: {
        printer.printAttribute("domainConfig", d_domainConfig.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* Response::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_FAILURE:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_FAILURE].name();
    case SELECTION_ID_DOMAIN_CONFIG:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_CONFIG].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -------------
// class Storage
// -------------

// CONSTANTS

const char Storage::CLASS_NAME[] = "Storage";

const bdlat_SelectionInfo Storage::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_IN_MEMORY,
     "inMemory",
     sizeof("inMemory") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_FILE_BACKED,
     "fileBacked",
     sizeof("fileBacked") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* Storage::lookupSelectionInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            Storage::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* Storage::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_IN_MEMORY:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_IN_MEMORY];
    case SELECTION_ID_FILE_BACKED:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_FILE_BACKED];
    default: return 0;
    }
}

// CREATORS

Storage::Storage(const Storage& original)
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_IN_MEMORY: {
        new (d_inMemory.buffer())
            InMemoryStorage(original.d_inMemory.object());
    } break;
    case SELECTION_ID_FILE_BACKED: {
        new (d_fileBacked.buffer())
            FileBackedStorage(original.d_fileBacked.object());
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Storage::Storage(Storage&& original) noexcept
: d_selectionId(original.d_selectionId)
{
    switch (d_selectionId) {
    case SELECTION_ID_IN_MEMORY: {
        new (d_inMemory.buffer())
            InMemoryStorage(bsl::move(original.d_inMemory.object()));
    } break;
    case SELECTION_ID_FILE_BACKED: {
        new (d_fileBacked.buffer())
            FileBackedStorage(bsl::move(original.d_fileBacked.object()));
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

Storage& Storage::operator=(const Storage& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_IN_MEMORY: {
            makeInMemory(rhs.d_inMemory.object());
        } break;
        case SELECTION_ID_FILE_BACKED: {
            makeFileBacked(rhs.d_fileBacked.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Storage& Storage::operator=(Storage&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_IN_MEMORY: {
            makeInMemory(bsl::move(rhs.d_inMemory.object()));
        } break;
        case SELECTION_ID_FILE_BACKED: {
            makeFileBacked(bsl::move(rhs.d_fileBacked.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void Storage::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_IN_MEMORY: {
        d_inMemory.object().~InMemoryStorage();
    } break;
    case SELECTION_ID_FILE_BACKED: {
        d_fileBacked.object().~FileBackedStorage();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int Storage::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_IN_MEMORY: {
        makeInMemory();
    } break;
    case SELECTION_ID_FILE_BACKED: {
        makeFileBacked();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int Storage::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

InMemoryStorage& Storage::makeInMemory()
{
    if (SELECTION_ID_IN_MEMORY == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_inMemory.object());
    }
    else {
        reset();
        new (d_inMemory.buffer()) InMemoryStorage();
        d_selectionId = SELECTION_ID_IN_MEMORY;
    }

    return d_inMemory.object();
}

InMemoryStorage& Storage::makeInMemory(const InMemoryStorage& value)
{
    if (SELECTION_ID_IN_MEMORY == d_selectionId) {
        d_inMemory.object() = value;
    }
    else {
        reset();
        new (d_inMemory.buffer()) InMemoryStorage(value);
        d_selectionId = SELECTION_ID_IN_MEMORY;
    }

    return d_inMemory.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
InMemoryStorage& Storage::makeInMemory(InMemoryStorage&& value)
{
    if (SELECTION_ID_IN_MEMORY == d_selectionId) {
        d_inMemory.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_inMemory.buffer()) InMemoryStorage(bsl::move(value));
        d_selectionId = SELECTION_ID_IN_MEMORY;
    }

    return d_inMemory.object();
}
#endif

FileBackedStorage& Storage::makeFileBacked()
{
    if (SELECTION_ID_FILE_BACKED == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_fileBacked.object());
    }
    else {
        reset();
        new (d_fileBacked.buffer()) FileBackedStorage();
        d_selectionId = SELECTION_ID_FILE_BACKED;
    }

    return d_fileBacked.object();
}

FileBackedStorage& Storage::makeFileBacked(const FileBackedStorage& value)
{
    if (SELECTION_ID_FILE_BACKED == d_selectionId) {
        d_fileBacked.object() = value;
    }
    else {
        reset();
        new (d_fileBacked.buffer()) FileBackedStorage(value);
        d_selectionId = SELECTION_ID_FILE_BACKED;
    }

    return d_fileBacked.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
FileBackedStorage& Storage::makeFileBacked(FileBackedStorage&& value)
{
    if (SELECTION_ID_FILE_BACKED == d_selectionId) {
        d_fileBacked.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_fileBacked.buffer()) FileBackedStorage(bsl::move(value));
        d_selectionId = SELECTION_ID_FILE_BACKED;
    }

    return d_fileBacked.object();
}
#endif

// ACCESSORS

bsl::ostream&
Storage::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_IN_MEMORY: {
        printer.printAttribute("inMemory", d_inMemory.object());
    } break;
    case SELECTION_ID_FILE_BACKED: {
        printer.printAttribute("fileBacked", d_fileBacked.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* Storage::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_IN_MEMORY:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_IN_MEMORY].name();
    case SELECTION_ID_FILE_BACKED:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_FILE_BACKED].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -------------
// class Request
// -------------

// CONSTANTS

const char Request::CLASS_NAME[] = "Request";

const bdlat_SelectionInfo Request::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_DOMAIN_CONFIG,
     "domainConfig",
     sizeof("domainConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo* Request::lookupSelectionInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            Request::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* Request::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_DOMAIN_CONFIG:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_CONFIG];
    default: return 0;
    }
}

// CREATORS

Request::Request(const Request& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN_CONFIG: {
        new (d_domainConfig.buffer())
            DomainConfigRequest(original.d_domainConfig.object(),
                                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Request::Request(Request&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN_CONFIG: {
        new (d_domainConfig.buffer())
            DomainConfigRequest(bsl::move(original.d_domainConfig.object()),
                                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

Request::Request(Request&& original, bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN_CONFIG: {
        new (d_domainConfig.buffer())
            DomainConfigRequest(bsl::move(original.d_domainConfig.object()),
                                d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

Request& Request::operator=(const Request& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DOMAIN_CONFIG: {
            makeDomainConfig(rhs.d_domainConfig.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Request& Request::operator=(Request&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DOMAIN_CONFIG: {
            makeDomainConfig(bsl::move(rhs.d_domainConfig.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void Request::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN_CONFIG: {
        d_domainConfig.object().~DomainConfigRequest();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int Request::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_DOMAIN_CONFIG: {
        makeDomainConfig();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int Request::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

DomainConfigRequest& Request::makeDomainConfig()
{
    if (SELECTION_ID_DOMAIN_CONFIG == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_domainConfig.object());
    }
    else {
        reset();
        new (d_domainConfig.buffer()) DomainConfigRequest(d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_CONFIG;
    }

    return d_domainConfig.object();
}

DomainConfigRequest&
Request::makeDomainConfig(const DomainConfigRequest& value)
{
    if (SELECTION_ID_DOMAIN_CONFIG == d_selectionId) {
        d_domainConfig.object() = value;
    }
    else {
        reset();
        new (d_domainConfig.buffer())
            DomainConfigRequest(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_CONFIG;
    }

    return d_domainConfig.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainConfigRequest& Request::makeDomainConfig(DomainConfigRequest&& value)
{
    if (SELECTION_ID_DOMAIN_CONFIG == d_selectionId) {
        d_domainConfig.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_domainConfig.buffer())
            DomainConfigRequest(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DOMAIN_CONFIG;
    }

    return d_domainConfig.object();
}
#endif

// ACCESSORS

bsl::ostream&
Request::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN_CONFIG: {
        printer.printAttribute("domainConfig", d_domainConfig.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* Request::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_DOMAIN_CONFIG:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DOMAIN_CONFIG].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------------
// class StorageDefinition
// -----------------------

// CONSTANTS

const char StorageDefinition::CLASS_NAME[] = "StorageDefinition";

const bdlat_AttributeInfo StorageDefinition::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_DOMAIN_LIMITS,
     "domainLimits",
     sizeof("domainLimits") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUE_LIMITS,
     "queueLimits",
     sizeof("queueLimits") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CONFIG,
     "config",
     sizeof("config") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
StorageDefinition::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StorageDefinition::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StorageDefinition::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_DOMAIN_LIMITS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOMAIN_LIMITS];
    case ATTRIBUTE_ID_QUEUE_LIMITS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_LIMITS];
    case ATTRIBUTE_ID_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG];
    default: return 0;
    }
}

// CREATORS

StorageDefinition::StorageDefinition()
: d_config()
, d_domainLimits()
, d_queueLimits()
{
}

StorageDefinition::StorageDefinition(const StorageDefinition& original)
: d_config(original.d_config)
, d_domainLimits(original.d_domainLimits)
, d_queueLimits(original.d_queueLimits)
{
}

StorageDefinition::~StorageDefinition()
{
}

// MANIPULATORS

StorageDefinition& StorageDefinition::operator=(const StorageDefinition& rhs)
{
    if (this != &rhs) {
        d_domainLimits = rhs.d_domainLimits;
        d_queueLimits  = rhs.d_queueLimits;
        d_config       = rhs.d_config;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StorageDefinition& StorageDefinition::operator=(StorageDefinition&& rhs)
{
    if (this != &rhs) {
        d_domainLimits = bsl::move(rhs.d_domainLimits);
        d_queueLimits  = bsl::move(rhs.d_queueLimits);
        d_config       = bsl::move(rhs.d_config);
    }

    return *this;
}
#endif

void StorageDefinition::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_domainLimits);
    bdlat_ValueTypeFunctions::reset(&d_queueLimits);
    bdlat_ValueTypeFunctions::reset(&d_config);
}

// ACCESSORS

bsl::ostream& StorageDefinition::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("domainLimits", this->domainLimits());
    printer.printAttribute("queueLimits", this->queueLimits());
    printer.printAttribute("config", this->config());
    printer.end();
    return stream;
}

// ------------------
// class Subscription
// ------------------

// CONSTANTS

const char Subscription::CLASS_NAME[] = "Subscription";

const bdlat_AttributeInfo Subscription::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_APP_ID,
     "appId",
     sizeof("appId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_EXPRESSION,
     "expression",
     sizeof("expression") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Subscription::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Subscription::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Subscription::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_APP_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID];
    case ATTRIBUTE_ID_EXPRESSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION];
    default: return 0;
    }
}

// CREATORS

Subscription::Subscription(bslma::Allocator* basicAllocator)
: d_appId(basicAllocator)
, d_expression(basicAllocator)
{
}

Subscription::Subscription(const Subscription& original,
                           bslma::Allocator*   basicAllocator)
: d_appId(original.d_appId, basicAllocator)
, d_expression(original.d_expression, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Subscription::Subscription(Subscription&& original) noexcept
: d_appId(bsl::move(original.d_appId)),
  d_expression(bsl::move(original.d_expression))
{
}

Subscription::Subscription(Subscription&&    original,
                           bslma::Allocator* basicAllocator)
: d_appId(bsl::move(original.d_appId), basicAllocator)
, d_expression(bsl::move(original.d_expression), basicAllocator)
{
}
#endif

Subscription::~Subscription()
{
}

// MANIPULATORS

Subscription& Subscription::operator=(const Subscription& rhs)
{
    if (this != &rhs) {
        d_appId      = rhs.d_appId;
        d_expression = rhs.d_expression;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Subscription& Subscription::operator=(Subscription&& rhs)
{
    if (this != &rhs) {
        d_appId      = bsl::move(rhs.d_appId);
        d_expression = bsl::move(rhs.d_expression);
    }

    return *this;
}
#endif

void Subscription::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_appId);
    bdlat_ValueTypeFunctions::reset(&d_expression);
}

// ACCESSORS

bsl::ostream&
Subscription::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("appId", this->appId());
    printer.printAttribute("expression", this->expression());
    printer.end();
    return stream;
}

// ------------
// class Domain
// ------------

// CONSTANTS

const char Domain::CLASS_NAME[] = "Domain";

const int Domain::DEFAULT_INITIALIZER_MAX_CONSUMERS = 0;

const int Domain::DEFAULT_INITIALIZER_MAX_PRODUCERS = 0;

const int Domain::DEFAULT_INITIALIZER_MAX_QUEUES = 0;

const int Domain::DEFAULT_INITIALIZER_MAX_IDLE_TIME = 0;

const int Domain::DEFAULT_INITIALIZER_MAX_DELIVERY_ATTEMPTS = 0;

const int Domain::DEFAULT_INITIALIZER_DEDUPLICATION_TIME_MS = 300000;

const bdlat_AttributeInfo Domain::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_MODE,
     "mode",
     sizeof("mode") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_STORAGE,
     "storage",
     sizeof("storage") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_MAX_CONSUMERS,
     "maxConsumers",
     sizeof("maxConsumers") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_PRODUCERS,
     "maxProducers",
     sizeof("maxProducers") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_QUEUES,
     "maxQueues",
     sizeof("maxQueues") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MSG_GROUP_ID_CONFIG,
     "msgGroupIdConfig",
     sizeof("msgGroupIdConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_MAX_IDLE_TIME,
     "maxIdleTime",
     sizeof("maxIdleTime") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MESSAGE_TTL,
     "messageTtl",
     sizeof("messageTtl") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_DELIVERY_ATTEMPTS,
     "maxDeliveryAttempts",
     sizeof("maxDeliveryAttempts") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_DEDUPLICATION_TIME_MS,
     "deduplicationTimeMs",
     sizeof("deduplicationTimeMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CONSISTENCY,
     "consistency",
     sizeof("consistency") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_SUBSCRIPTIONS,
     "subscriptions",
     sizeof("subscriptions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Domain::lookupAttributeInfo(const char* name,
                                                       int         nameLength)
{
    for (int i = 0; i < 13; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Domain::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Domain::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_MODE: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE];
    case ATTRIBUTE_ID_STORAGE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE];
    case ATTRIBUTE_ID_MAX_CONSUMERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONSUMERS];
    case ATTRIBUTE_ID_MAX_PRODUCERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_PRODUCERS];
    case ATTRIBUTE_ID_MAX_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_QUEUES];
    case ATTRIBUTE_ID_MSG_GROUP_ID_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_GROUP_ID_CONFIG];
    case ATTRIBUTE_ID_MAX_IDLE_TIME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_IDLE_TIME];
    case ATTRIBUTE_ID_MESSAGE_TTL:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_TTL];
    case ATTRIBUTE_ID_MAX_DELIVERY_ATTEMPTS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DELIVERY_ATTEMPTS];
    case ATTRIBUTE_ID_DEDUPLICATION_TIME_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DEDUPLICATION_TIME_MS];
    case ATTRIBUTE_ID_CONSISTENCY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSISTENCY];
    case ATTRIBUTE_ID_SUBSCRIPTIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS];
    default: return 0;
    }
}

// CREATORS

Domain::Domain(bslma::Allocator* basicAllocator)
: d_messageTtl()
, d_subscriptions(basicAllocator)
, d_name(basicAllocator)
, d_msgGroupIdConfig()
, d_storage()
, d_mode(basicAllocator)
, d_consistency()
, d_maxConsumers(DEFAULT_INITIALIZER_MAX_CONSUMERS)
, d_maxProducers(DEFAULT_INITIALIZER_MAX_PRODUCERS)
, d_maxQueues(DEFAULT_INITIALIZER_MAX_QUEUES)
, d_maxIdleTime(DEFAULT_INITIALIZER_MAX_IDLE_TIME)
, d_maxDeliveryAttempts(DEFAULT_INITIALIZER_MAX_DELIVERY_ATTEMPTS)
, d_deduplicationTimeMs(DEFAULT_INITIALIZER_DEDUPLICATION_TIME_MS)
{
}

Domain::Domain(const Domain& original, bslma::Allocator* basicAllocator)
: d_messageTtl(original.d_messageTtl)
, d_subscriptions(original.d_subscriptions, basicAllocator)
, d_name(original.d_name, basicAllocator)
, d_msgGroupIdConfig(original.d_msgGroupIdConfig)
, d_storage(original.d_storage)
, d_mode(original.d_mode, basicAllocator)
, d_consistency(original.d_consistency)
, d_maxConsumers(original.d_maxConsumers)
, d_maxProducers(original.d_maxProducers)
, d_maxQueues(original.d_maxQueues)
, d_maxIdleTime(original.d_maxIdleTime)
, d_maxDeliveryAttempts(original.d_maxDeliveryAttempts)
, d_deduplicationTimeMs(original.d_deduplicationTimeMs)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Domain::Domain(Domain&& original) noexcept
: d_messageTtl(bsl::move(original.d_messageTtl)),
  d_subscriptions(bsl::move(original.d_subscriptions)),
  d_name(bsl::move(original.d_name)),
  d_msgGroupIdConfig(bsl::move(original.d_msgGroupIdConfig)),
  d_storage(bsl::move(original.d_storage)),
  d_mode(bsl::move(original.d_mode)),
  d_consistency(bsl::move(original.d_consistency)),
  d_maxConsumers(bsl::move(original.d_maxConsumers)),
  d_maxProducers(bsl::move(original.d_maxProducers)),
  d_maxQueues(bsl::move(original.d_maxQueues)),
  d_maxIdleTime(bsl::move(original.d_maxIdleTime)),
  d_maxDeliveryAttempts(bsl::move(original.d_maxDeliveryAttempts)),
  d_deduplicationTimeMs(bsl::move(original.d_deduplicationTimeMs))
{
}

Domain::Domain(Domain&& original, bslma::Allocator* basicAllocator)
: d_messageTtl(bsl::move(original.d_messageTtl))
, d_subscriptions(bsl::move(original.d_subscriptions), basicAllocator)
, d_name(bsl::move(original.d_name), basicAllocator)
, d_msgGroupIdConfig(bsl::move(original.d_msgGroupIdConfig))
, d_storage(bsl::move(original.d_storage))
, d_mode(bsl::move(original.d_mode), basicAllocator)
, d_consistency(bsl::move(original.d_consistency))
, d_maxConsumers(bsl::move(original.d_maxConsumers))
, d_maxProducers(bsl::move(original.d_maxProducers))
, d_maxQueues(bsl::move(original.d_maxQueues))
, d_maxIdleTime(bsl::move(original.d_maxIdleTime))
, d_maxDeliveryAttempts(bsl::move(original.d_maxDeliveryAttempts))
, d_deduplicationTimeMs(bsl::move(original.d_deduplicationTimeMs))
{
}
#endif

Domain::~Domain()
{
}

// MANIPULATORS

Domain& Domain::operator=(const Domain& rhs)
{
    if (this != &rhs) {
        d_name                = rhs.d_name;
        d_mode                = rhs.d_mode;
        d_storage             = rhs.d_storage;
        d_maxConsumers        = rhs.d_maxConsumers;
        d_maxProducers        = rhs.d_maxProducers;
        d_maxQueues           = rhs.d_maxQueues;
        d_msgGroupIdConfig    = rhs.d_msgGroupIdConfig;
        d_maxIdleTime         = rhs.d_maxIdleTime;
        d_messageTtl          = rhs.d_messageTtl;
        d_maxDeliveryAttempts = rhs.d_maxDeliveryAttempts;
        d_deduplicationTimeMs = rhs.d_deduplicationTimeMs;
        d_consistency         = rhs.d_consistency;
        d_subscriptions       = rhs.d_subscriptions;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Domain& Domain::operator=(Domain&& rhs)
{
    if (this != &rhs) {
        d_name                = bsl::move(rhs.d_name);
        d_mode                = bsl::move(rhs.d_mode);
        d_storage             = bsl::move(rhs.d_storage);
        d_maxConsumers        = bsl::move(rhs.d_maxConsumers);
        d_maxProducers        = bsl::move(rhs.d_maxProducers);
        d_maxQueues           = bsl::move(rhs.d_maxQueues);
        d_msgGroupIdConfig    = bsl::move(rhs.d_msgGroupIdConfig);
        d_maxIdleTime         = bsl::move(rhs.d_maxIdleTime);
        d_messageTtl          = bsl::move(rhs.d_messageTtl);
        d_maxDeliveryAttempts = bsl::move(rhs.d_maxDeliveryAttempts);
        d_deduplicationTimeMs = bsl::move(rhs.d_deduplicationTimeMs);
        d_consistency         = bsl::move(rhs.d_consistency);
        d_subscriptions       = bsl::move(rhs.d_subscriptions);
    }

    return *this;
}
#endif

void Domain::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_mode);
    bdlat_ValueTypeFunctions::reset(&d_storage);
    d_maxConsumers = DEFAULT_INITIALIZER_MAX_CONSUMERS;
    d_maxProducers = DEFAULT_INITIALIZER_MAX_PRODUCERS;
    d_maxQueues    = DEFAULT_INITIALIZER_MAX_QUEUES;
    bdlat_ValueTypeFunctions::reset(&d_msgGroupIdConfig);
    d_maxIdleTime = DEFAULT_INITIALIZER_MAX_IDLE_TIME;
    bdlat_ValueTypeFunctions::reset(&d_messageTtl);
    d_maxDeliveryAttempts = DEFAULT_INITIALIZER_MAX_DELIVERY_ATTEMPTS;
    d_deduplicationTimeMs = DEFAULT_INITIALIZER_DEDUPLICATION_TIME_MS;
    bdlat_ValueTypeFunctions::reset(&d_consistency);
    bdlat_ValueTypeFunctions::reset(&d_subscriptions);
}

// ACCESSORS

bsl::ostream&
Domain::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("mode", this->mode());
    printer.printAttribute("storage", this->storage());
    printer.printAttribute("maxConsumers", this->maxConsumers());
    printer.printAttribute("maxProducers", this->maxProducers());
    printer.printAttribute("maxQueues", this->maxQueues());
    printer.printAttribute("msgGroupIdConfig", this->msgGroupIdConfig());
    printer.printAttribute("maxIdleTime", this->maxIdleTime());
    printer.printAttribute("messageTtl", this->messageTtl());
    printer.printAttribute("maxDeliveryAttempts", this->maxDeliveryAttempts());
    printer.printAttribute("deduplicationTimeMs", this->deduplicationTimeMs());
    printer.printAttribute("consistency", this->consistency());
    printer.printAttribute("subscriptions", this->subscriptions());
    printer.end();
    return stream;
}

// ----------------------
// class DomainDefinition
// ----------------------

// CONSTANTS

const char DomainDefinition::CLASS_NAME[] = "DomainDefinition";

const bdlat_AttributeInfo DomainDefinition::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_LOCATION,
     "location",
     sizeof("location") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PARAMETERS,
     "parameters",
     sizeof("parameters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
DomainDefinition::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DomainDefinition::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* DomainDefinition::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_LOCATION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCATION];
    case ATTRIBUTE_ID_PARAMETERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARAMETERS];
    default: return 0;
    }
}

// CREATORS

DomainDefinition::DomainDefinition(bslma::Allocator* basicAllocator)
: d_location(basicAllocator)
, d_parameters(basicAllocator)
{
}

DomainDefinition::DomainDefinition(const DomainDefinition& original,
                                   bslma::Allocator*       basicAllocator)
: d_location(original.d_location, basicAllocator)
, d_parameters(original.d_parameters, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainDefinition::DomainDefinition(DomainDefinition&& original) noexcept
: d_location(bsl::move(original.d_location)),
  d_parameters(bsl::move(original.d_parameters))
{
}

DomainDefinition::DomainDefinition(DomainDefinition&& original,
                                   bslma::Allocator*  basicAllocator)
: d_location(bsl::move(original.d_location), basicAllocator)
, d_parameters(bsl::move(original.d_parameters), basicAllocator)
{
}
#endif

DomainDefinition::~DomainDefinition()
{
}

// MANIPULATORS

DomainDefinition& DomainDefinition::operator=(const DomainDefinition& rhs)
{
    if (this != &rhs) {
        d_location   = rhs.d_location;
        d_parameters = rhs.d_parameters;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainDefinition& DomainDefinition::operator=(DomainDefinition&& rhs)
{
    if (this != &rhs) {
        d_location   = bsl::move(rhs.d_location);
        d_parameters = bsl::move(rhs.d_parameters);
    }

    return *this;
}
#endif

void DomainDefinition::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_location);
    bdlat_ValueTypeFunctions::reset(&d_parameters);
}

// ACCESSORS

bsl::ostream& DomainDefinition::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("location", this->location());
    printer.printAttribute("parameters", this->parameters());
    printer.end();
    return stream;
}

// -------------------
// class DomainVariant
// -------------------

// CONSTANTS

const char DomainVariant::CLASS_NAME[] = "DomainVariant";

const bdlat_SelectionInfo DomainVariant::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_DEFINITION,
     "definition",
     sizeof("definition") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {SELECTION_ID_REDIRECT,
     "redirect",
     sizeof("redirect") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_SelectionInfo* DomainVariant::lookupSelectionInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            DomainVariant::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* DomainVariant::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_DEFINITION:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_DEFINITION];
    case SELECTION_ID_REDIRECT:
        return &SELECTION_INFO_ARRAY[SELECTION_INDEX_REDIRECT];
    default: return 0;
    }
}

// CREATORS

DomainVariant::DomainVariant(const DomainVariant& original,
                             bslma::Allocator*    basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_DEFINITION: {
        new (d_definition.buffer())
            DomainDefinition(original.d_definition.object(), d_allocator_p);
    } break;
    case SELECTION_ID_REDIRECT: {
        new (d_redirect.buffer())
            bsl::string(original.d_redirect.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainVariant::DomainVariant(DomainVariant&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_DEFINITION: {
        new (d_definition.buffer())
            DomainDefinition(bsl::move(original.d_definition.object()),
                             d_allocator_p);
    } break;
    case SELECTION_ID_REDIRECT: {
        new (d_redirect.buffer())
            bsl::string(bsl::move(original.d_redirect.object()),
                        d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

DomainVariant::DomainVariant(DomainVariant&&   original,
                             bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_DEFINITION: {
        new (d_definition.buffer())
            DomainDefinition(bsl::move(original.d_definition.object()),
                             d_allocator_p);
    } break;
    case SELECTION_ID_REDIRECT: {
        new (d_redirect.buffer())
            bsl::string(bsl::move(original.d_redirect.object()),
                        d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

DomainVariant& DomainVariant::operator=(const DomainVariant& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DEFINITION: {
            makeDefinition(rhs.d_definition.object());
        } break;
        case SELECTION_ID_REDIRECT: {
            makeRedirect(rhs.d_redirect.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainVariant& DomainVariant::operator=(DomainVariant&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_DEFINITION: {
            makeDefinition(bsl::move(rhs.d_definition.object()));
        } break;
        case SELECTION_ID_REDIRECT: {
            makeRedirect(bsl::move(rhs.d_redirect.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void DomainVariant::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_DEFINITION: {
        d_definition.object().~DomainDefinition();
    } break;
    case SELECTION_ID_REDIRECT: {
        typedef bsl::string Type;
        d_redirect.object().~Type();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int DomainVariant::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_DEFINITION: {
        makeDefinition();
    } break;
    case SELECTION_ID_REDIRECT: {
        makeRedirect();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int DomainVariant::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

DomainDefinition& DomainVariant::makeDefinition()
{
    if (SELECTION_ID_DEFINITION == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_definition.object());
    }
    else {
        reset();
        new (d_definition.buffer()) DomainDefinition(d_allocator_p);
        d_selectionId = SELECTION_ID_DEFINITION;
    }

    return d_definition.object();
}

DomainDefinition& DomainVariant::makeDefinition(const DomainDefinition& value)
{
    if (SELECTION_ID_DEFINITION == d_selectionId) {
        d_definition.object() = value;
    }
    else {
        reset();
        new (d_definition.buffer()) DomainDefinition(value, d_allocator_p);
        d_selectionId = SELECTION_ID_DEFINITION;
    }

    return d_definition.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
DomainDefinition& DomainVariant::makeDefinition(DomainDefinition&& value)
{
    if (SELECTION_ID_DEFINITION == d_selectionId) {
        d_definition.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_definition.buffer())
            DomainDefinition(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_DEFINITION;
    }

    return d_definition.object();
}
#endif

bsl::string& DomainVariant::makeRedirect()
{
    if (SELECTION_ID_REDIRECT == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_redirect.object());
    }
    else {
        reset();
        new (d_redirect.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_REDIRECT;
    }

    return d_redirect.object();
}

bsl::string& DomainVariant::makeRedirect(const bsl::string& value)
{
    if (SELECTION_ID_REDIRECT == d_selectionId) {
        d_redirect.object() = value;
    }
    else {
        reset();
        new (d_redirect.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_REDIRECT;
    }

    return d_redirect.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& DomainVariant::makeRedirect(bsl::string&& value)
{
    if (SELECTION_ID_REDIRECT == d_selectionId) {
        d_redirect.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_redirect.buffer()) bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_REDIRECT;
    }

    return d_redirect.object();
}
#endif

// ACCESSORS

bsl::ostream&
DomainVariant::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_DEFINITION: {
        printer.printAttribute("definition", d_definition.object());
    } break;
    case SELECTION_ID_REDIRECT: {
        printer.printAttribute("redirect", d_redirect.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* DomainVariant::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_DEFINITION:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_DEFINITION].name();
    case SELECTION_ID_REDIRECT:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_REDIRECT].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}
}  // close package namespace
}  // close enterprise namespace

// GENERATED BY @BLP_BAS_CODEGEN_VERSION@
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbconfm --msgComponent messages mqbconf.xsd SERVICE
// VERSION bmqconf:183474-1.0
// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright 2024 Bloomberg Finance L.P. All rights reserved.
//      Property of Bloomberg Finance L.P. (BFLP)
//      This software is made available solely pursuant to the
//      terms of a BFLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
