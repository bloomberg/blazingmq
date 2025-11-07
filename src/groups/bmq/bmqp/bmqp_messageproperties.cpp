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

// bmqp_messageproperties.cpp                                         -*-C++-*-
#include <bmqp_messageproperties.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_protocolutil.h>

#include <bmqu_blob.h>
#include <bmqu_blobiterator.h>
#include <bmqu_blobobjectproxy.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlb_print.h>
#include <bdlb_scopeexit.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_algorithm.h>
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bsla_annotations.h>
#include <bslim_printer.h>
#include <bslma_default.h>
#include <bslmf_assert.h>

namespace BloombergLP {
namespace bmqp {

namespace {

// ===================================
// class PropertyValueStreamOutVisitor
// ===================================

/// This class provides operator() which enables it to be specified as a
/// visitor for the variant holding property value, and is used to stream
/// out the property value to the associated blob.
class PropertyValueStreamOutVisitor {
  private:
    // DATA
    bdlbb::Blob* d_blob_p;  // Blob to stream out the property value held in
                            // the variant.

  public:
    // CREATORS
    explicit PropertyValueStreamOutVisitor(bdlbb::Blob* blob)
    : d_blob_p(blob)
    {
        // PRECONDITIONS
        BSLS_ASSERT(d_blob_p);
    }

    // MANIPULATORS
    void operator()(bool value)
    {
        char val = value ? 1 : 0;
        bdlbb::BlobUtil::append(d_blob_p, &val, 1);
    }

    void operator()(char value)
    {
        bdlbb::BlobUtil::append(d_blob_p, &value, 1);
    }

    void operator()(short value)
    {
        bdlb::BigEndianInt16 nboValue = bdlb::BigEndianInt16::make(value);
        bdlbb::BlobUtil::append(d_blob_p,
                                reinterpret_cast<char*>(&nboValue),
                                sizeof(nboValue));
    }

    void operator()(int value)
    {
        bdlb::BigEndianInt32 nboValue = bdlb::BigEndianInt32::make(value);
        bdlbb::BlobUtil::append(d_blob_p,
                                reinterpret_cast<char*>(&nboValue),
                                sizeof(nboValue));
    }

    void operator()(bsls::Types::Int64 value)
    {
        bdlb::BigEndianInt64 nboValue = bdlb::BigEndianInt64::make(value);
        bdlbb::BlobUtil::append(d_blob_p,
                                reinterpret_cast<char*>(&nboValue),
                                sizeof(nboValue));
    }

    void operator()(const bsl::string_view& value)
    {
        bdlbb::BlobUtil::append(d_blob_p,
                                value.data(),
                                static_cast<int>(value.length()));
    }

    void operator()(const bsl::vector<char>& value)
    {
        bdlbb::BlobUtil::append(d_blob_p,
                                value.data(),
                                static_cast<int>(value.size()));
    }
};

bool validatePropertyLength(int propLen, bmqt::PropertyType::Enum propType)
{
    // PRECONDITIONS
    BSLS_ASSERT(bmqt::PropertyType::e_UNDEFINED != propType);

    switch (propType) {
    case bmqt::PropertyType::e_BOOL: {
        if (propLen != sizeof(char)) {
            return false;  // RETURN
        }
        return true;  // RETURN
    }
    case bmqt::PropertyType::e_CHAR: {
        if (propLen != sizeof(char)) {
            return false;  // RETURN
        }
        return true;  // RETURN
    }
    case bmqt::PropertyType::e_SHORT: {
        if (propLen != sizeof(bdlb::BigEndianInt16)) {
            return false;  // RETURN
        }
        return true;  // RETURN
    }
    case bmqt::PropertyType::e_INT32: {
        if (propLen != sizeof(bdlb::BigEndianInt32)) {
            return false;  // RETURN
        }
        return true;  // RETURN
    }
    case bmqt::PropertyType::e_INT64: {
        if (propLen != sizeof(bdlb::BigEndianInt64)) {
            return false;  // RETURN
        }
        return true;  // RETURN
    }
    case bmqt::PropertyType::e_STRING: {
        return true;  // RETURN
    }
    case bmqt::PropertyType::e_BINARY: {
        return true;  // RETURN
    }
    case bmqt::PropertyType::e_UNDEFINED:
    default:
        BSLS_ASSERT_OPT(0 && "Invalid data type for property value.");
        break;  // BREAK
    }

    return false;
}

}  // close unnamed namespace

// ==============================
// class MessageProperties_Schema
// ==============================

// CREATORS
MessageProperties_Schema::MessageProperties_Schema(
    const MessageProperties& mps,
    bslma::Allocator*        basicAllocator)
: d_indices(basicAllocator)
{
    bmqp::MessagePropertiesIterator it(&mps);

    for (int index = 0; it.hasNext(); ++index) {  // starts with '0'
        d_indices.emplace(it.name(), index);
    }
}

MessageProperties_Schema::MessageProperties_Schema(
    const MessageProperties_Schema& other)
: d_indices(other.d_indices)
{
    // NOTHING
}

MessageProperties_Schema::~MessageProperties_Schema()
{
    // NOTHING
}

// PUBLIC ACCESSORS
bool MessageProperties_Schema::loadIndex(int*               index,
                                         const bsl::string& name) const
{
    PropertyMap::const_iterator cit = d_indices.find(name);

    if (cit == d_indices.end()) {
        // Unknown property
        return false;  // RETURN
    }

    *index = cit->second;

    return true;
}

// -----------------------
// class MessageProperties
// -----------------------

// PRIVATE ACCESSORS
const MessageProperties::PropertyVariant&
MessageProperties::getPropertyValue(const Property& property) const
{
    if (property.d_value.isUnset()) {
        BSLA_MAYBE_UNUSED bool result = streamInPropertyValue(property);
        BSLS_ASSERT_SAFE(result);
        // We assert 'true' result because the length and offset have already
        // been checked.
    }
    return property.d_value;
}

const MessageProperties::PropertyVariant&
MessageProperties::getPropertyValueAsString(const Property& property) const
{
    if (property.d_value.isUnset()) {
        BSLA_MAYBE_UNUSED bool result = streamInPropertyValue(property);
        BSLS_ASSERT_SAFE(result);
        // We assert 'true' result because the length and offset have already
        // been checked.
    }
    PropertyVariant& v = property.d_value;

    if (v.is<bsl::string_view>()) {
        v = bsl::string(v.the<bsl::string_view>(), d_allocator_p);
    }

    return v;
}

bool MessageProperties::streamInPropertyValue(const Property& p) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(bmqt::PropertyType::e_UNDEFINED != p.d_type);
    BSLS_ASSERT_SAFE(p.d_value.isUnset());
    BSLS_ASSERT_SAFE(p.d_offset);

    bmqu::BlobPosition position;
    int rc = bmqu::BlobUtil::findOffsetSafe(&position, *d_blob_p, p.d_offset);
    BSLS_ASSERT_SAFE(rc == 0);

    switch (p.d_type) {
    case bmqt::PropertyType::e_BOOL: {
        char value;
        rc = bmqu::BlobUtil::readNBytes(&value,
                                        *d_blob_p,
                                        position,
                                        sizeof(value));

        p.d_value = value == 1 ? true : false;
        break;
    }
    case bmqt::PropertyType::e_CHAR: {
        char value;
        rc = bmqu::BlobUtil::readNBytes(&value,
                                        *d_blob_p,
                                        position,
                                        sizeof(value));

        p.d_value = value;
        break;
    }
    case bmqt::PropertyType::e_SHORT: {
        bdlb::BigEndianInt16 nboValue;
        rc = bmqu::BlobUtil::readNBytes(reinterpret_cast<char*>(&nboValue),
                                        *d_blob_p,
                                        position,
                                        sizeof(nboValue));

        p.d_value = static_cast<short>(nboValue);
        break;
    }
    case bmqt::PropertyType::e_INT32: {
        bdlb::BigEndianInt32 nboValue;
        rc = bmqu::BlobUtil::readNBytes(reinterpret_cast<char*>(&nboValue),
                                        *d_blob_p,
                                        position,
                                        sizeof(nboValue));

        p.d_value = static_cast<int>(nboValue);
        break;
    }
    case bmqt::PropertyType::e_INT64: {
        bdlb::BigEndianInt64 nboValue;
        rc = bmqu::BlobUtil::readNBytes(reinterpret_cast<char*>(&nboValue),
                                        *d_blob_p,
                                        position,
                                        sizeof(nboValue));

        p.d_value = static_cast<bsls::Types::Int64>(nboValue);
        break;
    }

    case bmqt::PropertyType::e_STRING: {
        // Try to avoid copying the string.  'd_blop' already keeps a copy.
        bmqu::BlobPosition end;
        int                ret =
            bmqu::BlobUtil::findOffset(&end, *d_blob_p, position, p.d_length);
        bool doCopy = true;
        if (ret == 0) {
            // Do not align
            if (position.buffer() == end.buffer() ||
                (position.buffer() + 1 == end.buffer() && end.byte() == 0)) {
                // Section is good
                char* start = d_blob_p->buffer(position.buffer()).data() +
                              position.byte();

                p.d_value = bsl::string_view(start, p.d_length);
                doCopy    = false;
            }
        }
        if (doCopy) {
            bsl::string value(p.d_length, ' ', d_allocator_p);
            rc        = bmqu::BlobUtil::readNBytes(&value[0],
                                            *d_blob_p,
                                            position,
                                            p.d_length);
            p.d_value = value;
        }

        break;
    }
    case bmqt::PropertyType::e_BINARY: {
        bsl::vector<char> value(p.d_length, d_allocator_p);
        rc = bmqu::BlobUtil::readNBytes(&value[0],
                                        *d_blob_p,
                                        position,
                                        p.d_length);

        p.d_value = value;
        break;
    }
    case bmqt::PropertyType::e_UNDEFINED:
    default:
        BSLS_ASSERT_OPT(0 && "Invalid data type for property value.");
        rc = -1;
        break;  // BREAK
    }

    return rc == 0;
}

// CREATORS
MessageProperties::MessageProperties(bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_properties(basicAllocator)
, d_totalSize(0)
, d_originalSize(0)
, d_blob()
, d_blob_p(d_blob.address())
, d_isBlobConstructed(false)
, d_isDirty(true)  // by default, this should be true
, d_mphSize(0)
, d_mphOffset(0)
, d_numProps(0)
, d_dataOffset(0)
, d_schema()
, d_originalNumProps(0)
, d_doDeepCopy(true)
{
}

MessageProperties::MessageProperties(const MessageProperties& other,
                                     bslma::Allocator*        basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_properties(other.d_properties, basicAllocator)
, d_totalSize(other.d_totalSize)
, d_originalSize(other.d_originalSize)
, d_blob()
, d_blob_p(d_blob.address())
, d_isBlobConstructed(false)
, d_isDirty(other.d_isDirty)
, d_mphSize(other.d_mphSize)
, d_mphOffset(other.d_mphOffset)
, d_numProps(other.d_numProps)
, d_dataOffset(other.d_dataOffset)
, d_schema(other.d_schema)
, d_originalNumProps(other.d_originalNumProps)
, d_doDeepCopy(other.d_doDeepCopy)
{
    if (other.d_isBlobConstructed) {
        new (d_blob.buffer())
            bdlbb::Blob(other.d_blob.object(), d_allocator_p);
        d_isBlobConstructed = true;
    }
}

MessageProperties::~MessageProperties()
{
    clear();
}

// MANIPULATORS
MessageProperties& MessageProperties::operator=(const MessageProperties& rhs)
{
    if (this == &rhs) {
        return *this;  // RETURN
    }

    d_properties   = rhs.d_properties;
    d_totalSize    = rhs.d_totalSize;
    d_originalSize = rhs.d_originalSize;
    d_isDirty      = rhs.d_isDirty;

    if (d_isBlobConstructed) {
        d_blob.object().bdlbb::Blob::~Blob();
        d_isBlobConstructed = false;
    }

    if (rhs.d_isBlobConstructed) {
        new (d_blob.buffer()) bdlbb::Blob(rhs.d_blob.object(), d_allocator_p);
        d_blob_p            = d_blob.address();
        d_isBlobConstructed = true;
    }

    d_mphSize          = rhs.d_mphSize;
    d_mphOffset        = rhs.d_mphOffset;
    d_numProps         = rhs.d_numProps;
    d_dataOffset       = rhs.d_dataOffset;
    d_schema           = rhs.d_schema;
    d_originalNumProps = rhs.d_originalNumProps;

    return *this;
}

void MessageProperties::clear()
{
    d_properties.clear();
    d_totalSize    = 0;
    d_originalSize = 0;
    d_isDirty      = true;
    d_mphSize      = 0;
    d_mphOffset    = 0;
    d_numProps     = 0;
    d_dataOffset   = 0;
    d_schema.clear();

    d_originalNumProps = 0;

    if (d_isBlobConstructed) {
        d_blob.object().bdlbb::Blob::~Blob();
        d_isBlobConstructed = false;
    }
}

bool MessageProperties::remove(const bsl::string&        name,
                               bmqt::PropertyType::Enum* buffer)
{
    PropertyMapIter it = findProperty(name);
    if (it == d_properties.end()) {
        return false;  // RETURN
    }

    BSLS_ASSERT(0 < totalSize());

    Property& p = it->second;

    bmqt::PropertyType::Enum ptype = p.d_type;
    BSLS_ASSERT(bmqt::PropertyType::e_UNDEFINED != ptype);
    if (buffer) {
        *buffer = ptype;
    }

    // Decrement 'd_totalSize' by property value's length.

    d_totalSize -= p.d_length;

    BSLS_ASSERT(0 < totalSize());

    // Decrement 'd_totalSize' by property name's length and length of struct
    // 'MessagePropertyHeader'.

    d_totalSize -= static_cast<int>(it->first.length());
    d_totalSize -= static_cast<int>(sizeof(MessagePropertyHeader));

    if (p.d_offset) {
        // Cannot remove the property since reading other properties needs
        // the 'd_offset'.

        p.d_isValid = false;
        p.d_value.reset();
    }
    else {
        d_properties.erase(it);
    }

    BSLS_ASSERT_SAFE(d_numProps > 0);

    --d_numProps;

    if (0 == numProperties()) {
        // Even if this was the last property, 'd_totalSize' should still be
        // greater than zero because of 'struct MessagePropertiesHeader'.

        BSLS_ASSERT(0 < totalSize());
        d_totalSize = 0;
    }

    d_isDirty = true;
    return true;
}

int MessageProperties::streamInHeader(const bdlbb::Blob& blob)
{
    clear();

    d_isDirty = true;

    if (0 == blob.length()) {
        // Empty blob implies no message properties.  This is a valid wire
        // representation.  This instance has already been cleared above, which
        // represents an empty instance.

        return rc_SUCCESS;  // RETURN
    }

    // Specified 'blob' must be in the same format as the wire protocol.  So it
    // should start with a MessagePropertiesHeader followed by one or more
    // MessagePropertyHeader followed by one or more (name, value) pairs, and
    // end with padding for word alignment.

    // Read 'MessagePropertiesHeader'.
    bmqu::BlobObjectProxy<MessagePropertiesHeader> msgPropsHeader(
        &blob,
        -MessagePropertiesHeader::k_MIN_HEADER_SIZE,
        true,    // read flag
        false);  // write flag
    if (!msgPropsHeader.isSet()) {
        return rc_NO_MSG_PROPERTIES_HEADER;  // RETURN
    }

    msgPropsHeader.resize(msgPropsHeader->headerSize());
    if (!msgPropsHeader.isSet()) {
        return rc_INCOMPLETE_MSG_PROPERTIES_HEADER;  // RETURN
    }

    const int msgPropsAreaSize = msgPropsHeader->messagePropertiesAreaWords() *
                                 Protocol::k_WORD_SIZE;

    if (msgPropsAreaSize > blob.length()) {
        return rc_INCORRECT_LENGTH;  // RETURN
    }

    d_mphSize    = msgPropsHeader->messagePropertyHeaderSize();
    d_mphOffset  = msgPropsHeader->headerSize();
    d_numProps   = msgPropsHeader->numProperties();
    d_dataOffset = d_mphOffset + d_numProps * d_mphSize;
    if (0 >= d_mphSize) {
        return rc_INVALID_MPH_SIZE;  // RETURN
    }

    if (0 >= d_numProps || k_MAX_NUM_PROPERTIES < d_numProps) {
        return rc_INVALID_NUM_PROPERTIES;  // RETURN
    }

    // Note that 'd_totalSize' represents length of wire-representation, but
    // excludes the word-aligned padding present at the end of properties
    // area.  So we update 'd_totalSize' accordingly.

    d_totalSize = ProtocolUtil::calcUnpaddedLength(blob, msgPropsAreaSize);

    if (d_totalSize > blob.length()) {
        return rc_INCORRECT_LENGTH;  // RETURN
    }

    if (d_doDeepCopy) {
        new (d_blob.buffer()) bdlbb::Blob(d_allocator_p);
        bdlbb::BlobUtil::append(d_blob.address(), blob, 0, d_totalSize);
        d_blob_p            = d_blob.address();
        d_isBlobConstructed = true;
    }
    else {
        d_blob_p = &blob;
    }
    d_originalSize      = d_totalSize;
    d_originalNumProps  = d_numProps;

    return rc_SUCCESS;
}

int MessageProperties::streamInPropertyHeader(Property*    property,
                                              bsl::string* name,
                                              Property*    previous,
                                              int*         totalLength,
                                              bool isNewStyleProperties,
                                              int  start,
                                              int  index) const
{
    BSLS_ASSERT_SAFE(property);
    BSLS_ASSERT_SAFE(totalLength);
    BSLS_ASSERT_SAFE(d_dataOffset && start);
    BSLS_ASSERT_SAFE(d_blob_p);
    BSLS_ASSERT_SAFE(d_isBlobConstructed || d_blob_p != d_blob.address());

    bmqu::BlobPosition position;

    if (bmqu::BlobUtil::findOffsetSafe(&position, *d_blob_p, start)) {
        // Failed to advance blob to next 'MessagePropertyHeader' location.
        return rc_NO_MSG_PROPERTY_HEADER;  // RETURN
    }

    bmqu::BlobObjectProxy<MessagePropertyHeader> mpHeader(
        d_blob_p,
        position,
        d_mphSize,
        true,    // read flag
        false);  // write flag

    // Note that above, we use size specified in 'MessagePropertiesHeader',
    // not sizeof(MessagePropertyHeader).

    if (!mpHeader.isSet()) {
        return rc_INCOMPLETE_MSG_PROPERTY_HEADER;  // RETURN
    }

    // Keep track of property's info.
    const bmqt::PropertyType::Enum type =
        static_cast<bmqt::PropertyType::Enum>(mpHeader->propertyType());
    int  length   = 0;
    int  nameLen  = mpHeader->propertyNameLength();
    int  valueLen = mpHeader->propertyValueLength();
    bool isValid  = true;
    int  offset   = 0;

    if (bmqt::PropertyType::e_BOOL > type ||
        bmqt::PropertyType::e_BINARY < type) {
        return rc_INVALID_PROPERTY_TYPE;  // RETURN
    }

    if (k_MAX_PROPERTY_NAME_LENGTH < nameLen) {
        return rc_INVALID_PROPERTY_NAME_LENGTH;  // RETURN
    }

    if (isNewStyleProperties) {
        // New style.  Calculate length as delta between offsets.
        offset = d_dataOffset + valueLen;  // offset to the property name

        if (previous == 0) {
            if (index == 0 && valueLen) {
                // The first property's offset must be '0'.
                return rc_INVALID_PROPERTY_VALUE_LENGTH;  // RETURN
            }
            // Schema may read one MP without reading all.
            *totalLength = offset;
        }
        else {
            // Calculate the previous length as delta between offsets.
            if (offset < previous->d_offset) {
                return rc_INVALID_PROPERTY_VALUE_LENGTH;  // RETURN
            }
            int previousLenghtOnTheWire = offset - previous->d_offset;

            if (previous->d_length == 0) {
                previous->d_length = previousLenghtOnTheWire;

                isValid = validatePropertyLength(previous->d_length,
                                                 previous->d_type);
            }
            // else the 'previous' has been modified explicitly or read.

            *totalLength += previousLenghtOnTheWire;
        }

        if (index == (d_originalNumProps - 1)) {
            // The last property.
            // Calculate the length as delta between the total and offset.
            // Use 'd_originalSize' for 'd_totalSize' can be modified.

            if (d_originalSize < (offset + nameLen)) {
                return rc_INVALID_PROPERTY_VALUE_LENGTH;  // RETURN
            }
            length = d_originalSize - offset - nameLen;
        }
    }
    else {
        // Old style.
        length = valueLen;
        // Assume that the old style incrementally reads all properties
        offset = *totalLength;
    }

    if (property->d_length == 0) {
        // Old style property or the last property which was not modified.
        property->d_type = type;

        if (length) {
            isValid = isValid && validatePropertyLength(length, type);
            property->d_length = length;
        }
    }
    // else, this property has been modified explicitly
    property->d_offset = offset + nameLen;

    if (!isValid) {
        return rc_INVALID_PROPERTY_VALUE_LENGTH;  // RETURN
    }

    *totalLength += length + nameLen;
    // The binary protocol carries offsets to names, but in-memory
    // struct keeps offsets to values (+ nameLen).

    if (k_MAX_PROPERTY_VALUE_LENGTH < length) {
        return rc_INVALID_PROPERTY_VALUE_LENGTH;  // RETURN
    }

    // Use 'd_originalSize' for 'd_totalSize' can be modified.
    if (index == (d_originalNumProps - 1) && d_originalSize != *totalLength) {
        return rc_INCORRECT_LENGTH;  // RETURN
    }

    if (name) {
        // Read the property name.  Do not read the value (lazy read).
        // Validate the type though to make sure later reading won't fail.
        // Future reads will assert on error.

        name->assign(nameLen, ' ');
        bmqu::BlobPosition namePosition;
        int                rc = bmqu::BlobUtil::findOffsetSafe(&namePosition,
                                                *d_blob_p,
                                                offset);
        if (rc) {
            return rc_MISSING_PROPERTY_AREA;  // RETURN
        }

        rc = bmqu::BlobUtil::readNBytes(name->begin(),
                                        *d_blob_p,
                                        namePosition,
                                        nameLen);
        if (rc) {
            return 100 * rc + rc_PROPERTY_NAME_STREAMIN_FAILURE;  // RETURN
        }
    }

    return rc_SUCCESS;
}

int MessageProperties::loadProperties(bool isFirstTime,
                                      bool isNewStyleProperties) const
{
    // Iterate over all 'MessagePropertyHeader' fields and keep track of
    // relevant values, to avoid a second pass over the blob.

    int       totalLength = d_dataOffset;
    Property* previous    = 0;
    int       start       = d_mphOffset;

    for (int i = 0; i < d_originalNumProps; ++i, start += d_mphSize) {
        // Move the blob position to the beginning of 'MessagePropertyHeader'.

        bsl::string name;
        Property    next;
        int         rc = streamInPropertyHeader(&next,
                                        &name,
                                        previous,
                                        &totalLength,
                                        isNewStyleProperties,
                                        start,
                                        i);
        if (rc) {
            return rc;
        }

        PropertyMapInsertRc lookupOrInsert = d_properties.insert(
            bsl::make_pair(name, next));

        if (!lookupOrInsert.second && isFirstTime) {
            return rc_DUPLICATE_PROPERTY_NAME;  // RETURN
        }

        previous = &lookupOrInsert.first->second;
    }

    return rc_SUCCESS;
}

int MessageProperties::streamIn(const bdlbb::Blob& blob,
                                bool               isNewStyleProperties)
{
    clear();

    bdlb::ScopeExitAny cleaner(
        bdlf::BindUtil::bind(&MessageProperties::clear, this));

    int rc = streamInHeader(blob);

    if (rc != rc_SUCCESS) {
        return rc;  // RETURN
    }

    if (blob.length() < d_dataOffset) {
        return rc_MISSING_MSG_PROPERTY_HEADERS;  // RETURN
    }

    // TODO: This could always build schema on the fly if it is missing to
    // avoid extra iteration over properties in subsequent 'getSchema'.

    rc = loadProperties(true, isNewStyleProperties);
    if (rc != rc_SUCCESS) {
        return rc;  // RETURN
    }

    cleaner.release();

    return rc_SUCCESS;
}

int MessageProperties::streamIn(const bdlbb::Blob&           blob,
                                const MessagePropertiesInfo& info,
                                const SchemaPtr&             schema)
{
    int rc;
    if (schema) {
        rc = streamInHeader(blob);
        if (rc == 0) {
            d_schema = schema;
        }
    }
    else {
        rc = streamIn(blob, info.isExtended());
    }
    return rc;
}

// ACCESSORS
// PUBLIC ACCESSORS
bdld::Datum
MessageProperties::getPropertyRef(const bsl::string& name,
                                  bslma::Allocator*  basicAllocator) const
{
    PropertyMapIter it = findProperty(name);
    if (it == d_properties.end()) {
        return bdld::Datum::createError(-1);  // RETURN
    }
    const Property& property = it->second;

    const PropertyVariant& v = getPropertyValue(property);
    switch (property.d_type) {
    case bmqt::PropertyType::e_BOOL:
        return bdld::Datum::createBoolean(v.the<bool>());
    case bmqt::PropertyType::e_CHAR:
        return bdld::Datum::createInteger(v.the<char>());
    case bmqt::PropertyType::e_SHORT:
        return bdld::Datum::createInteger(v.the<short>());
    case bmqt::PropertyType::e_INT32:
        return bdld::Datum::createInteger(v.the<int>());
    case bmqt::PropertyType::e_INT64:
        return bdld::Datum::createInteger64(v.the<bsls::Types::Int64>(),
                                            basicAllocator);
    case bmqt::PropertyType::e_STRING:
        if (v.is<bsl::string>()) {
            return bdld::Datum::createStringRef(v.the<bsl::string>(),
                                                basicAllocator);
        }
        else {
            return bdld::Datum::createStringRef(v.the<bsl::string_view>(),
                                                basicAllocator);
        }
    case bmqt::PropertyType::e_BINARY:
        // do not want to use binary
        return bdld::Datum::createError(-2);
    case bmqt::PropertyType::e_UNDEFINED:
    default: return bdld::Datum::createError(-3);
    }
}

bool MessageProperties::hasProperty(const bsl::string&        name,
                                    bmqt::PropertyType::Enum* type) const
{
    PropertyMapConstIter cit = findProperty(name);
    if (cit == d_properties.end()) {
        return false;  // RETURN
    }

    if (type) {
        *type = cit->second.d_type;
    }

    return true;
}

bmqt::PropertyType::Enum
MessageProperties::propertyType(const bsl::string& name) const
{
    PropertyMapConstIter cit = findProperty(name);
    BSLS_ASSERT((cit != d_properties.end()) && "Property does not exist");

    return cit->second.d_type;
}

const bdlbb::Blob&
MessageProperties::streamOut(bdlbb::BlobBufferFactory*          bufferFactory,
                             const bmqp::MessagePropertiesInfo& info) const
{
    if (!d_isDirty) {
        BSLS_ASSERT(d_isBlobConstructed);
        return d_blob.object();  // RETURN
    }

    // Make sure all Properties are read.
    for (PropertyMapConstIter cit = d_properties.begin();
         cit != d_properties.end();
         ++cit) {
        const Property& p = cit->second;
        if (p.d_value.isUnset() && p.d_isValid) {
            BSLA_MAYBE_UNUSED bool result = streamInPropertyValue(p);
            BSLS_ASSERT(result);
        }
    }

    if (d_isBlobConstructed) {
        // This instance is dirty, which means it will have to be streamed out
        // to 'd_blob'.  Destroy the current instance of blob, which may be
        // having a null blob buffer factory associated with it.  This is
        // possible if 'd_blob' was initialized in the 'streamIn' routine.

        d_blob.object().bdlbb::Blob::~Blob();
        d_isBlobConstructed = false;
    }

    new (d_blob.buffer()) bdlbb::Blob(bufferFactory, d_allocator_p);

    d_blob_p            = d_blob.address();
    d_isBlobConstructed = true;

    if (0 == numProperties()) {
        BSLS_ASSERT(0 == totalSize());

        d_blob.object().removeAll();

        d_isDirty = false;
        return d_blob.object();  // RETURN
    }

    BSLS_ASSERT(k_MAX_NUM_PROPERTIES >= numProperties());
    BSLS_ASSERT(k_MAX_PROPERTIES_AREA_LENGTH >= totalSize());

    bdlbb::Blob* blob = &(d_blob.object());

    // Add MessagePropertiesHeader.  Since the buffer factory being used by the
    // 'blob' could have been provided by the user, we cannot assume that
    // buffer size of the associated blob buffer factory is not less than the
    // sizeof(bmqp::MessagePropertiesHeader), otherwise we could have simply
    // invoked 'setLength' on the blob and reinterpret_casted first few bytes
    // of the first buffer to 'bmqp::MessagePropertiesHeader'.

    bmqu::BlobUtil::reserve(blob, sizeof(MessagePropertiesHeader));
    bmqu::BlobObjectProxy<MessagePropertiesHeader> msgPropsHeader(
        blob,
        false,  // read flag
        true);  // write flag
    BSLS_ASSERT(msgPropsHeader.isSet());

    new (msgPropsHeader.object()) MessagePropertiesHeader();
    msgPropsHeader->setNumProperties(numProperties());

    msgPropsHeader.reset();  // write out the header.
    int totalSize = sizeof(MessagePropertiesHeader);

    // Add MessagePropertHeader structs, one for each property.
    int offset = 0;

    for (PropertyMapConstIter cit = d_properties.begin();
         cit != d_properties.end();
         ++cit) {
        const Property& p = cit->second;

        if (!p.d_isValid) {
            continue;  // CONTINUE
        }

        BSLS_ASSERT(!p.d_value.isUnset());

        bmqu::BlobPosition pos;
        bmqu::BlobUtil::reserve(&pos, blob, sizeof(MessagePropertyHeader));
        bmqu::BlobObjectProxy<MessagePropertyHeader> msgPropHeader(
            blob,
            pos,
            false,  // read flag
            true);  // write flag
        new (msgPropHeader.object()) MessagePropertyHeader();
        msgPropHeader->setPropertyType(p.d_type);
        msgPropHeader->setPropertyNameLength(
            static_cast<int>(cit->first.length()));
        if (info.isExtended()) {
            msgPropHeader->setPropertyValueLength(offset);
        }
        else {
            msgPropHeader->setPropertyValueLength(cit->second.d_length);
        }

        msgPropHeader.reset();  // write out the header
        totalSize += sizeof(MessagePropertyHeader);
        offset += static_cast<int>(cit->first.length());
        offset += p.d_length;
    }

    BSLS_ASSERT(totalSize < d_totalSize);

    // Now add property name and value individually.

    for (PropertyMapConstIter cit = d_properties.begin();
         cit != d_properties.end();
         ++cit) {
        const Property& p = cit->second;

        if (!p.d_isValid) {
            continue;  // CONTINUE
        }
        // Append property name.

        bdlbb::BlobUtil::append(blob,
                                cit->first.c_str(),
                                static_cast<int>(cit->first.length()));
        totalSize += static_cast<int>(cit->first.length());

        // Append property value.

        PropertyValueStreamOutVisitor visitor(blob);
        p.d_value.applyRaw(visitor);
        totalSize += p.d_length;
    }

    BSLS_ASSERT(totalSize == d_totalSize);
    BSLS_ASSERT(totalSize <= k_MAX_PROPERTIES_AREA_LENGTH);

    // Per contract, blob must contain padding.

    int padding  = 0;
    int numWords = ProtocolUtil::calcNumWordsAndPadding(&padding, totalSize);
    ProtocolUtil::appendPaddingRaw(blob, padding);

    // Update the 'total message properties area words' field in the top-level
    // struct 'MessagePropertiesHeader'.

    bmqu::BlobObjectProxy<MessagePropertiesHeader> msgPropsHdr(
        blob,
        false,  // read flag
        true);  // write flag
    BSLS_ASSERT(msgPropsHdr.isSet());
    msgPropsHdr->setMessagePropertiesAreaWords(numWords);
    msgPropsHdr.reset();
    d_isDirty = false;

    return *d_blob_p;
}

bsl::ostream& MessageProperties::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    static const size_t k_MAX_BYTES_DUMP = 256;

    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();

    MessagePropertiesIterator msgPropIter(this);

    while (msgPropIter.hasNext()) {
        bdlma::LocalSequentialAllocator<64> nameLsa(0);
        bmqu::MemOutStream                  nameOs(&nameLsa);

        nameOs << msgPropIter.name() << " (" << msgPropIter.type() << ")"
               << bsl::ends;

        switch (msgPropIter.type()) {
        case bmqt::PropertyType::e_BOOL: {
            printer.printAttribute(nameOs.str().data(),
                                   msgPropIter.getAsBool());
        } break;
        case bmqt::PropertyType::e_CHAR: {
            bmqu::OutStreamFormatSaver fmtSaver(stream);
            stream << bsl::hex;
            printer.printAttribute(nameOs.str().data(),
                                   static_cast<int>(msgPropIter.getAsChar()));
        } break;
        case bmqt::PropertyType::e_SHORT: {
            printer.printAttribute(nameOs.str().data(),
                                   msgPropIter.getAsShort());
        } break;
        case bmqt::PropertyType::e_INT32: {
            printer.printAttribute(nameOs.str().data(),
                                   msgPropIter.getAsInt32());
        } break;
        case bmqt::PropertyType::e_INT64: {
            printer.printAttribute(nameOs.str().data(),
                                   msgPropIter.getAsInt64());
        } break;
        case bmqt::PropertyType::e_STRING: {
            printer.printAttribute(nameOs.str().data(),
                                   msgPropIter.getAsString());
        } break;
        case bmqt::PropertyType::e_BINARY: {
            const bsl::vector<char>& binaryVec = msgPropIter.getAsBinary();
            size_t printSize = bsl::min(binaryVec.size(), k_MAX_BYTES_DUMP);

            bdlma::LocalSequentialAllocator<k_MAX_BYTES_DUMP> lsa(0);

            bmqu::MemOutStream os(k_MAX_BYTES_DUMP, &lsa);
            os << bdlb::PrintStringHexDumper(&binaryVec[0],
                                             static_cast<int>(printSize));

            printer.printAttribute(nameOs.str().data(), os.str());
        } break;
        case bmqt::PropertyType::e_UNDEFINED:
        default: printer.printValue("* invalid message property *");
        }
    }

    printer.end();

    return stream;
}

// -------------------------------
// class MessagePropertiesIterator
// -------------------------------

// MANIPULATORS
bool MessagePropertiesIterator::hasNext()
{
    BSLS_ASSERT_SAFE(d_properties_p);

    if (d_first) {
        d_iterator = d_properties_p->d_properties.begin();
        d_first    = false;
    }
    else {
        BSLS_ASSERT_SAFE(d_iterator != d_properties_p->d_properties.end());
        ++d_iterator;
    }

    if (d_iterator == d_properties_p->d_properties.end()) {
        return false;  // RETURN
    }

    // Skip the property if it's first character is not an alphanumeric, which
    // may represent a system property.

    const bsl::string& propName = name();
    if (!propName.empty() && !bsl::isalnum(propName[0])) {
        return hasNext();  // RETURN
    }

    // Skip the property if it has been removed
    if (!d_iterator->second.d_isValid) {
        return hasNext();  // RETURN
    }

    return true;
}

}  // close package namespace
}  // close enterprise namespace
