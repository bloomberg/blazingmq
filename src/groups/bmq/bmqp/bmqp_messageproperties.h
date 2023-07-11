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

// bmqp_messageproperties.h                                           -*-C++-*-
#ifndef INCLUDED_BMQP_MESSAGEPROPERTIES
#define INCLUDED_BMQP_MESSAGEPROPERTIES

//@PURPOSE: Provide a VST representing message properties.
//
//@CLASSES:
//  bmqp::MessageProperties: VST representing message properties.
//
//@SEE ALSO: bmqt::PropertyType
//

// BMQ

#include <bmqp_protocol.h>
#include <bmqt_propertytype.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlb_variant.h>
#include <bdlbb_blob.h>
#include <bdld_datum.h>
#include <bsl_cctype.h>
#include <bsl_cstddef.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_objectbuffer.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bmqp {

// FORWARD DECLARATION
class MessageProperties;
class MessagePropertiesIterator;

// ==============================
// class MessageProperties_Schema
// ==============================

/// This VST keeps a knowledge about unique sequence of Properties.  The
/// goal is to return the index of a given property.
/// Once created, it is read-only.
class MessageProperties_Schema {
  private:
    // PRIVATE TYPES

    /// The choice between HashTable and Map affects
    /// `MessagePropertiesIterator` if we want the same iteration order
    /// in both schema-optimized (lazy Properties reading) and non-optimized
    /// cases.
    /// Map is the same type as `MessageProperties::d_properties` so an
    /// iterator could use either `Schema` or `MessageProperties`.
    /// HashTable has better performance but an iterator requires
    /// populating `MessageProperties::d_properties` upon `begin()`.
    /// The choice is HashTable.
    typedef bsl::unordered_map<bsl::string, int> PropertyMap;

  private:
    // PRIVATE DATA
    PropertyMap d_indices;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MessageProperties_Schema,
                                   bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create Schema from the specified `mps`.  Once created, it cannot
    /// change
    MessageProperties_Schema(const MessageProperties& mps,
                             bslma::Allocator*        basicAllocator);
    MessageProperties_Schema(const MessageProperties_Schema& other);

    // PUBLIC ACCESSORS
    bool loadIndex(int* index, const bsl::string& name) const;
};

// =======================
// class MessageProperties
// =======================

/// Provide a VST representing message properties.
class MessageProperties {
    // FRIENDS
    friend class MessagePropertiesIterator;

  private:
    // PRIVATE TYPES

    /// IMPLEMENTATION NOTE: The order of types in above variant *must* be
    /// same as the order of `bmqt::PropertyType::Enum`, because at certain
    /// places, PropertyVariant.typeIndex() is cast into
    /// `bmqt::PropertyType::Enum`.  If it is desired to change one of the
    /// orders, property type must be explicitly maintained as a separate
    /// field in the `d_properties`.
    typedef bdlb::Variant7<bool,
                           char,
                           short,
                           int,
                           bsls::Types::Int64,
                           bsl::string,
                           bsl::vector<char> >
        PropertyVariant;

    struct Property {
        int d_offset;
        // Offset in the blob to the value (not the
        // name) or '0' for newly added properties.

        int d_length;
        // Length of the value (not the name)

        bmqt::PropertyType::Enum d_type;
        // Type of the value.  Available even if
        // the value is 'unSet'.

        mutable PropertyVariant d_value;
        // Property value; gets read on on demand.

        bool d_isValid;
        // If the property is removed, it stays in
        // the collection to keep its 'd_offset'.

        Property();
    };

    /// PropertyName -> (PropertySize, PropertyValue) map.  Note that
    /// although property's size can be retrieved from property's value (by
    /// applying a visitor to the variant), size is explicitly maintained to
    /// avoid switch cases during serialization.
    typedef bsl::map<bsl::string, Property> PropertyMap;

    typedef PropertyMap::iterator PropertyMapIter;

    typedef PropertyMap::const_iterator PropertyMapConstIter;

    typedef bsl::pair<PropertyMapIter, bool> PropertyMapInsertRc;

    typedef bsls::ObjectBuffer<bdlbb::Blob> BlobObjectBuffer;

    enum RcEnum {
        rc_SUCCESS                          = 0,
        rc_NO_MSG_PROPERTIES_HEADER         = -1,
        rc_INCOMPLETE_MSG_PROPERTIES_HEADER = -2,
        rc_INCORRECT_LENGTH                 = -3,
        rc_INVALID_MPH_SIZE                 = -4,
        rc_INVALID_NUM_PROPERTIES           = -5,
        rc_MISSING_MSG_PROPERTY_HEADERS     = -6,
        rc_NO_MSG_PROPERTY_HEADER           = -7,
        rc_INCOMPLETE_MSG_PROPERTY_HEADER   = -8,
        rc_INVALID_PROPERTY_TYPE            = -9,
        rc_INVALID_PROPERTY_NAME_LENGTH     = -10,
        rc_INVALID_PROPERTY_VALUE_LENGTH    = -11,
        rc_MISSING_PROPERTY_AREA            = -12,
        rc_PROPERTY_NAME_STREAMIN_FAILURE   = -13,
        rc_DUPLICATE_PROPERTY_NAME          = -14
    };

  public:
    // PUBLIC TYPES
    typedef MessageProperties_Schema      Schema;
    typedef bsl::shared_ptr<const Schema> SchemaPtr;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    mutable PropertyMap d_properties;
    // Hash table containing property
    // name->value pairs.

    int d_totalSize;
    // Total size of the BlazingMQ wire
    // protocol representation of the
    // properties including their names,
    // values and protocol overhead, but
    // *excluding* the word-aligned padding
    // which goes at the end of the
    // properties area.

    int d_originalSize;
    // The total size read from the wire
    // _before_ any property can change to
    // use when reading incrementally.

    mutable BlobObjectBuffer d_blob;  // Wire representation.

    mutable bool d_isBlobConstructed;
    // Flag indicating if an instance of
    // the blob has been constructed in
    // 'd_blob'.

    mutable bool d_isDirty;
    // Flag indicating if this instance has
    // been updated since the previous
    // invocation of 'streamOut()'.

    int d_mphSize;
    // Size of MessagePropertyHeader
    int d_mphOffset;
    // Offset to 1st MessagePropertyHeader
    int d_numProps;

    int d_dataOffset;
    // start of names and values

    SchemaPtr d_schema;

    mutable int d_lastError;

    int d_originalNumProps;
    // The count as read from the wire
    // _before_ any property can change.
    // Incremental reading needs it to
    // recognize last property.

  private:
    // PRIVATE CLASS METHODS

    /// Return true if the specified `name` represents a valid property
    /// name, and false otherwise.  See `Restrictions on Property Names`
    /// section for what constitutes a valid name.
    static bool isValidPropertyName(const bsl::string& name);

    // PRIVATE MANIPULATORS

    /// Set the property with the specified `name` to the specified `value`
    /// and return an enum representing the status of this operation.
    template <class TYPE>
    bmqt::GenericResult::Enum setProperty(const bsl::string& name,
                                          const TYPE&        value);

    /// Parse just the `MessagePropertiesHeader` out of the specified
    /// `blob`.  Return `0` on success and initialize internal state ready
    /// to read individual `MessagePropertysHeader` structs.
    int streamInHeader(const bdlbb::Blob& blob);

    // PRIVATE ACCESSORS

    /// Return the size of the specified property `value` with the specified
    /// `TYPE`.
    template <class TYPE>
    int getPropertyValueSize(const TYPE& value) const;

    /// Return the value of the specified `property`.
    const PropertyVariant& getPropertyValue(const Property& property) const;

    /// Return the value of the property with the specified `name`.
    /// Behavior is undefined unless a property with `name` exists and the
    /// value is of the same `TYPE`.
    template <class TYPE>
    const TYPE& getProperty(const bsl::string& name) const;

    /// Return the value of the property with the specified `name` if a
    /// property with such name exists, otherwise return the specified
    /// `value`.  Behavior is undefined unless data type of property's value
    /// is same as the specified `TYPE`.
    template <class TYPE>
    const TYPE& getPropertyOr(const bsl::string& name,
                              const TYPE&        value) const;

    /// Stream into the specified property `p` the value of the property
    /// according to previously parsed length and position.  Return `true`
    /// on success, `false` otherwise.
    bool streamInPropertyValue(const Property& p) const;

    PropertyMapIter findProperty(const bsl::string& name) const;

    /// Parse one `MessagePropertyHeader` out of the specified `blob` at the
    /// specified `offset`, at the specified `index`, using the specified
    /// `isNewStyleProperties` as an indicator of encoding style.
    /// Accumulate number of processed bytes in the specified `totalLength`.
    /// Use the specified `previous` to calculate length of (previous)
    /// property value if the encoding is in the new style.  If the
    /// specified `name` is not `0`, read and load property name.  Load
    /// results of parsing into the specified `next`.  The behavior is
    /// undefined without prior successful `streamInHeader` call.
    /// Return 0 on success, non-zero error code otherwise.
    int streamInPropertyHeader(Property*    next,
                               bsl::string* name,
                               Property*    previous,
                               int*         totalLength,
                               bool         isNewStyleProperties,
                               int          offset,
                               int          index) const;

  public:
    // PUBLIC CONSTANTS
    static const int k_MAX_NUM_PROPERTIES =
        MessagePropertiesHeader::k_MAX_NUM_PROPERTIES;

    static const int k_MAX_PROPERTIES_AREA_LENGTH =
        MessagePropertiesHeader::k_MAX_MESSAGE_PROPERTIES_SIZE -
        Protocol::k_WORD_SIZE;
    // Maximum length of all the properties (including their names, values
    // and the wire protocol overhead, but excluding the word-aligned
    // padding).  Note that this value is just under 64 MB.

    /// Maximum length of a property name.
    static const int k_MAX_PROPERTY_NAME_LENGTH =
        MessagePropertyHeader::k_MAX_PROPERTY_NAME_LENGTH;

    static const int k_MAX_PROPERTY_VALUE_LENGTH =
        (MessageProperties::k_MAX_PROPERTIES_AREA_LENGTH -
         MessageProperties::k_MAX_PROPERTY_NAME_LENGTH -
         sizeof(MessagePropertiesHeader) - sizeof(MessagePropertyHeader) -
         Protocol::k_WORD_SIZE);  // 67104745 bytes (~64 MB)
    // Maximum length of a property value.  Note that this value is just
    // under 64 MB.  Also note that this value is calculated assuming that
    // there is only one property and property's name has maximum allowable
    // length, and also takes into consideration the protocol overhead.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MessageProperties,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an empty instance of `MessageProperties`.  Optionally
    /// specify a `basicAllocator` used to supply memory.  It is undefined
    /// behavior to attempt to associate this instance with a message.
    explicit MessageProperties(bslma::Allocator* basicAllocator = 0);

    /// Create an instance of `MessageProperties` having the same value as
    /// the specified `other`, that will use the optionally specified
    /// `basicAllocator` to supply memory.
    MessageProperties(const MessageProperties& other,
                      bslma::Allocator*        basicAllocator = 0);

    /// Destroy this object.
    ~MessageProperties();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    MessageProperties& operator=(const MessageProperties& rhs);

    /// Remove the property with the specified `name` if one exists and
    /// return true and load into the optionally specified `buffer` the data
    /// type of the property.  Return false if property with `name` does not
    /// exist, and leave `buffer` unchanged.
    bool remove(const bsl::string& name, bmqt::PropertyType::Enum* buffer = 0);

    /// Remove all properties from this instance.  Note that `numProperties`
    /// will return zero after invoking this method.
    void clear();

    int setPropertyAsBool(const bsl::string& name, bool value);
    int setPropertyAsChar(const bsl::string& name, char value);
    int setPropertyAsShort(const bsl::string& name, short value);
    int setPropertyAsInt32(const bsl::string& name, int value);
    int setPropertyAsInt64(const bsl::string& name, bsls::Types::Int64 value);
    int setPropertyAsString(const bsl::string& name, const bsl::string& value);

    /// Set a property with the specified `name` having the specified
    /// `value` with the corresponding data type.  Return zero on success,
    /// and a non-zero value in case of failure.  Note that if a property
    /// with the `name` and same type exists, it will be updated with
    /// `value`, however if the data type of the existing property differs,
    /// an error will be returned.
    int setPropertyAsBinary(const bsl::string&       name,
                            const bsl::vector<char>& value);

    /// Populate this instance with its BlazingMQ wire protocol
    /// representation from the specified `blob`.  If the specified
    /// `isNewStyleProperties` is `true`, use the new format of encoding
    /// offset instead of length in each MessagePropertyHader.  Return zero
    /// on success, and a non-zero value otherwise.
    int streamIn(const bdlbb::Blob& blob, bool isNewStyleProperties);

    /// Parse `MessagePropertiesHeader` out of the specified `blob` using
    /// the specified `info` as an indicator of encoding format.  If the
    /// specified `schema` is not empty, return without parsing properties
    /// headers.  Otherwise, populate this instance with properties names,
    /// lengths, types, and offsets.
    /// Return zero on success, and a non-zero value otherwise.
    int streamIn(const bdlbb::Blob&           blob,
                 const MessagePropertiesInfo& info,
                 const SchemaPtr&             schema);

    /// Parse and load all previously unparsed properties headers using the
    /// specified `isNewStyleProperties` as an indicator of encoding style.
    /// (Properties headers are not parsed in the presence of schema unless
    /// explicitly loaded).  If the specified `isFirstTime` is `true`,
    /// return an error on duplicate property name.
    /// Return 0 on success, non-zero error code otherwise.
    int loadProperties(bool isFirstTime, bool isNewStyleProperties) const;

    // ACCESSORS

    /// Return the total number of properties set in this instance.
    int numProperties() const;

    /// Return the total size (in bytes) of the wire representation of this
    /// instance.  Note that returned value includes the BlazingMQ wire
    /// protocol overhead as well.
    int totalSize() const;

    /// Return true if a property with the specified `name` exists and load
    /// into the optionally specified `type` the type of the property.
    /// Return false otherwise.
    bool hasProperty(const bsl::string&        name,
                     bmqt::PropertyType::Enum* type = 0) const;

    /// Return the type of property having the specified `name`.  Behavior
    /// is undefined unless `hasProperty` returns true for the specified
    /// property `name`.
    bmqt::PropertyType::Enum propertyType(const bsl::string& name) const;

    bool               getPropertyAsBool(const bsl::string& name) const;
    char               getPropertyAsChar(const bsl::string& name) const;
    short              getPropertyAsShort(const bsl::string& name) const;
    int                getPropertyAsInt32(const bsl::string& name) const;
    bsls::Types::Int64 getPropertyAsInt64(const bsl::string& name) const;
    const bsl::string& getPropertyAsString(const bsl::string& name) const;

    /// Return the property having the corresponding type and the specified
    /// `name`.  Behavior is undefined unless property with `name` exists.
    const bsl::vector<char>&
    getPropertyAsBinary(const bsl::string& name) const;

    bool  getPropertyAsBoolOr(const bsl::string& name, bool value) const;
    char  getPropertyAsCharOr(const bsl::string& name, char value) const;
    short getPropertyAsShortOr(const bsl::string& name, short value) const;
    int   getPropertyAsInt32Or(const bsl::string& name, int value) const;
    bsls::Types::Int64 getPropertyAsInt64Or(const bsl::string& name,
                                            bsls::Types::Int64 value) const;
    const bsl::string& getPropertyAsStringOr(const bsl::string& name,
                                             const bsl::string& value) const;
    const bsl::vector<char>&
    getPropertyAsBinaryOr(const bsl::string&       name,
                          const bsl::vector<char>& value) const;

    bdld::Datum getPropertyRef(const bsl::string& name,
                               bslma::Allocator*  basicAllocator) const;

    // Return a reference to the property with the specified 'name' if
    // property with such a name exists.  Return 'bdld::Datum::createError'
    // if property with 'name' does not exist.  Behavior is undefined when
    // accessing the returned reference after this object changes its
    // state.

    SchemaPtr makeSchema(bslma::Allocator* allocator);

    /// Return a blob having the BlazingMQ wire protocol representation of
    /// this instance.  The specified `info` controls MessagePropertyHeader
    /// encoding - `extended info` means new format of encoding offsets
    /// instead of property lengths.  Behavior is undefined unless specified
    /// `bufferFactory` is non-null.  Note that if this instance is empty
    /// (i.e.,`numProperties()` == 0), returned blob will be empty.  In
    /// other words, an empty instance has no wire representation.
    const bdlbb::Blob&
    streamOut(bdlbb::BlobBufferFactory*          bufferFactory,
              const bmqp::MessagePropertiesInfo& info) const;

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
bsl::ostream& operator<<(bsl::ostream& stream, const MessageProperties& rhs);

// ===============================
// class MessagePropertiesIterator
// ===============================

/// Provide a mechanism to iterator over all the properties in an instance
/// of `bmqp::MessageProperties`.  The order of the iteration is
/// implementation defined.  An iterator is *valid* if it is associated with
/// a property , otherwise it is *invalid*.  Behavior is undefined if the
/// underlying instance of `bmqp::MessageProperties` is modified during the
/// lifetime of this iterator.
class MessagePropertiesIterator {
  private:
    // PRIVATE TYPES
    typedef MessageProperties::PropertyVariant PropertyVariant;

    typedef MessageProperties::PropertyMap PropertyMap;

    typedef MessageProperties::PropertyMapConstIter PropertyMapConstIter;

  private:
    // DATA
    const MessageProperties* d_properties_p;
    // Message properties instance backing
    // this iterator.

    PropertyMapConstIter d_iterator;
    // Current position in
    // 'd_properties_p'.

    bool d_first;  // Flag indicating if its the first
                   // iteration.

  public:
    // CREATORS

    /// Create an empty iterator instance.  The only valid methods that can
    /// be invoked on an empty instance are the assignment operator and the
    /// destructor.
    MessagePropertiesIterator();

    /// Create an iterator for the specified `properties`.  Behavior is
    /// undefined unless `properties` is not null.
    explicit MessagePropertiesIterator(const MessageProperties* properties);

    // MANIPULATORS

    /// Advance this iterator to refer to the next property of the
    /// associated `MessageProperties` instance, if there is one and return
    /// true, return false otherwise.  Behavior is undefined unless this
    /// method is being invoked for the first time on this object or
    /// previous call to `hasNext` returned true.  Note that the order of
    /// the iteration is not specified.
    bool hasNext();

    // ACCESSORS

    /// Return a reference not offering modifiable access to the name of the
    /// property being pointed by the iterator.  Behavior is undefined
    /// unless last call to `hasNext` returned true.
    const bsl::string& name() const;

    /// Return the data type of property being pointed by the iterator.
    /// Behavior is undefined unless last call to `hasNext` returned true;
    bmqt::PropertyType::Enum type() const;

    bool               getAsBool() const;
    char               getAsChar() const;
    short              getAsShort() const;
    int                getAsInt32() const;
    bsls::Types::Int64 getAsInt64() const;
    const bsl::string& getAsString() const;

    /// Return property value having the corresponding type being currently
    /// being pointed by this iterator instance.  Behavior is undefined
    /// unless last call to `hasNext` returned true.  Behavior is also
    /// undefined unless property's data type matches the requested type.
    const bsl::vector<char>& getAsBinary() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------
// class MessageProperties::Property
// ---------------------------------
inline MessageProperties::Property::Property()
: d_offset(0)
, d_length(0)
, d_type(bmqt::PropertyType::e_UNDEFINED)
, d_value()
, d_isValid(true)
{
    // NOTHING
}
// -----------------------
// class MessageProperties
// -----------------------

// PRIVATE CLASS LEVEL METHODS
inline bool MessageProperties::isValidPropertyName(const bsl::string& name)
{
    if (name.empty()) {
        return false;  // RETURN
    }

    if (k_MAX_PROPERTY_NAME_LENGTH < name.length()) {
        return false;  // RETURN
    }

    // TBD: use C++ flavor of 'isalnum' instead, which takes the locale as
    // well?
    return bsl::isalnum(name[0]);
}

// PRIVATE MANIPULATORS
template <class TYPE>
bmqt::GenericResult::Enum
MessageProperties::setProperty(const bsl::string& name, const TYPE& value)
{
    // PRECONDITIONS
    BSLS_ASSERT(k_MAX_PROPERTIES_AREA_LENGTH >= totalSize());

    if (!isValidPropertyName(name)) {
        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    if (k_MAX_NUM_PROPERTIES == numProperties()) {
        return bmqt::GenericResult::e_REFUSED;  // RETURN
    }

    const int newPropValueLen = getPropertyValueSize(value);

    if (k_MAX_PROPERTY_VALUE_LENGTH < newPropValueLen) {
        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    int delta = newPropValueLen;

    PropertyMapIter it = findProperty(name);
    if (it != d_properties.end()) {
        Property& existing = it->second;

        if (!getPropertyValue(existing).is<TYPE>()) {
            return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
        }

        delta -= existing.d_length;

        BSLS_ASSERT(0 < totalSize());
        d_isDirty = true;
    }
    else {
        delta += name.length() + sizeof(MessagePropertyHeader);
        if (0 == numProperties()) {
            // If its the 1st property, also include the size of struct
            // 'MessagePropertiesHeader'.

            BSLS_ASSERT(0 == totalSize());
            delta += sizeof(MessagePropertiesHeader);
        }
    }

    if (k_MAX_PROPERTIES_AREA_LENGTH < (totalSize() + delta)) {
        return bmqt::GenericResult::e_REFUSED;  // RETURN
    }

    // All checks have passed, commit
    if (it == d_properties.end()) {
        ++d_numProps;
    }

    d_totalSize += delta;

    PropertyMapInsertRc insertRc = d_properties.insert(
        bsl::make_pair(name, Property()));
    Property& p = insertRc.first->second;

    p.d_length  = newPropValueLen;
    p.d_value   = value;
    p.d_type    = static_cast<bmqt::PropertyType::Enum>(p.d_value.typeIndex());
    p.d_isValid = true;
    d_isDirty   = true;

    return bmqt::GenericResult::e_SUCCESS;
}

// PRIVATE ACCESSORS
template <class TYPE>
inline int MessageProperties::getPropertyValueSize(
    BSLS_ANNOTATION_UNUSED const TYPE& value) const
{
    return static_cast<int>(sizeof(TYPE));
}

template <>
inline int MessageProperties::getPropertyValueSize(
    BSLS_ANNOTATION_UNUSED const bool& value) const
{
    // Partial specialization for type 'bool', to return 1.  sizeof(bool)
    // should be one on all platforms, but just in case.

    return 1;
}

template <>
inline int
MessageProperties::getPropertyValueSize(const bsl::string& value) const
{
    // Partial specialization for type 'bsl::string'.

    return static_cast<int>(value.length());
}

template <>
inline int
MessageProperties::getPropertyValueSize(const bsl::vector<char>& value) const
{
    // Partial specialization for type 'bsl::vector<char>'.

    return static_cast<int>(value.size());
}

template <class TYPE>
inline const TYPE&
MessageProperties::getProperty(const bsl::string& name) const
{
    PropertyMapConstIter cit = findProperty(name);
    BSLS_ASSERT((cit != d_properties.end()) && "Property does not exist");

    const PropertyVariant& value = getPropertyValue(cit->second);

    BSLS_ASSERT(value.is<TYPE>() && "Property data type mismatch");

    return value.the<TYPE>();
}

template <class TYPE>
inline const TYPE& MessageProperties::getPropertyOr(const bsl::string& name,
                                                    const TYPE& value) const
{
    PropertyMapConstIter cit = findProperty(name);
    if (cit == d_properties.end()) {
        return value;  // RETURN
    }
    const Property& p = cit->second;
    if (p.d_value.isUnset()) {
        bool result = streamInPropertyValue(p);
        BSLS_ASSERT(result);
        // We assert 'true' result because the length and offset have already
        // been checked.
        (void)result;
    }
    BSLS_ASSERT(p.d_value.is<TYPE>() && "Property data type mismatch");

    return p.d_value.the<TYPE>();
}

inline MessageProperties::PropertyMapIter
MessageProperties::findProperty(const bsl::string& name) const
{
    PropertyMapIter cit = d_properties.find(name);
    if (cit == d_properties.end()) {
        if (!d_schema) {
            return cit;  // RETURN
        }
    }
    else if (cit->second.d_isValid) {
        return cit;  // RETURN
    }
    else {
        // Removed property
        return d_properties.end();  // RETURN
    }

    BSLS_ASSERT_SAFE(d_schema);

    int index;
    if (d_schema->loadIndex(&index, name.c_str())) {
        // Starts with '0'

        Property theProperty;
        Property next;
        int      totalLength = d_dataOffset;
        int      offset      = d_mphOffset + index * d_mphSize;
        int      rc          = 0;
        // We have to call twice (unless this is the last property) to
        // calculate 'theProperty' length from two offsets.

        bsl::string temp;  // Can be '0'; using it to double-check
        rc = streamInPropertyHeader(&theProperty,
                                    &temp,
                                    0,
                                    &totalLength,
                                    true,
                                    offset,
                                    index);
        BSLS_ASSERT_SAFE(name == temp);
        if (rc) {
            // REVISIT: there are no means to report the error other than
            //          returning 'end()'
            d_lastError = rc;
            return d_properties.end();  // RETURN
        }

        if (index < (d_originalNumProps - 1)) {
            rc = streamInPropertyHeader(&next,
                                        0,
                                        &theProperty,
                                        &totalLength,
                                        true,
                                        offset + d_mphSize,
                                        index + 1);
            if (rc) {
                // REVISIT: there is no means to report the error other than
                //          returning 'end()'
                return d_properties.end();  // RETURN
            }
        }

        PropertyMapInsertRc insert = d_properties.insert(
            bsl::make_pair(name, theProperty));
        BSLS_ASSERT_SAFE(insert.second);
        cit = insert.first;
    }

    return cit;
}

// MANIPULATORS
inline int MessageProperties::setPropertyAsBool(const bsl::string& name,
                                                bool               value)
{
    return setProperty(name, value);
}

inline int MessageProperties::setPropertyAsChar(const bsl::string& name,
                                                char               value)
{
    return setProperty(name, value);
}

inline int MessageProperties::setPropertyAsShort(const bsl::string& name,
                                                 short              value)
{
    return setProperty(name, value);
}

inline int MessageProperties::setPropertyAsInt32(const bsl::string& name,
                                                 int                value)
{
    return setProperty(name, value);
}

inline int MessageProperties::setPropertyAsInt64(const bsl::string& name,
                                                 bsls::Types::Int64 value)
{
    return setProperty(name, value);
}

inline int MessageProperties::setPropertyAsString(const bsl::string& name,
                                                  const bsl::string& value)
{
    return setProperty(name, value);
}

inline int
MessageProperties::setPropertyAsBinary(const bsl::string&       name,
                                       const bsl::vector<char>& value)
{
    return setProperty(name, value);
}

// ACCESSORS

inline MessageProperties::SchemaPtr
MessageProperties::makeSchema(bslma::Allocator* allocator)
{
    if (!d_schema) {
        d_schema.load(new (*allocator)
                          MessageProperties_Schema(*this, allocator),
                      allocator);
    }
    return d_schema;
}

inline bool MessageProperties::getPropertyAsBool(const bsl::string& name) const
{
    return getProperty<bool>(name);
}

inline char MessageProperties::getPropertyAsChar(const bsl::string& name) const
{
    return getProperty<char>(name);
}

inline short
MessageProperties::getPropertyAsShort(const bsl::string& name) const
{
    return getProperty<short>(name);
}

inline int MessageProperties::getPropertyAsInt32(const bsl::string& name) const
{
    return getProperty<int>(name);
}

inline bsls::Types::Int64
MessageProperties::getPropertyAsInt64(const bsl::string& name) const
{
    return getProperty<bsls::Types::Int64>(name);
}

inline const bsl::string&
MessageProperties::getPropertyAsString(const bsl::string& name) const
{
    return getProperty<bsl::string>(name);
}

inline const bsl::vector<char>&
MessageProperties::getPropertyAsBinary(const bsl::string& name) const
{
    return getProperty<bsl::vector<char> >(name);
}

inline bool MessageProperties::getPropertyAsBoolOr(const bsl::string& name,
                                                   bool value) const
{
    return getPropertyOr(name, value);
}

inline char MessageProperties::getPropertyAsCharOr(const bsl::string& name,
                                                   char value) const
{
    return getPropertyOr(name, value);
}

inline short MessageProperties::getPropertyAsShortOr(const bsl::string& name,
                                                     short value) const
{
    return getPropertyOr(name, value);
}

inline int MessageProperties::getPropertyAsInt32Or(const bsl::string& name,
                                                   int value) const
{
    return getPropertyOr(name, value);
}

inline bsls::Types::Int64
MessageProperties::getPropertyAsInt64Or(const bsl::string& name,
                                        bsls::Types::Int64 value) const
{
    return getPropertyOr(name, value);
}

inline const bsl::string&
MessageProperties::getPropertyAsStringOr(const bsl::string& name,
                                         const bsl::string& value) const
{
    return getPropertyOr(name, value);
}

inline const bsl::vector<char>&
MessageProperties::getPropertyAsBinaryOr(const bsl::string&       name,
                                         const bsl::vector<char>& value) const
{
    return getPropertyOr(name, value);
}

inline int MessageProperties::totalSize() const
{
    return d_totalSize;
}

inline int MessageProperties::numProperties() const
{
    return d_numProps;
}

// -------------------------------
// class MessagePropertiesIterator
// -------------------------------

// CREATORS
inline MessagePropertiesIterator::MessagePropertiesIterator()
: d_properties_p(0)
, d_iterator()
, d_first(true)
{
    // NOTHING
}

inline MessagePropertiesIterator::MessagePropertiesIterator(
    const MessageProperties* properties)
: d_properties_p(properties)
, d_iterator()
, d_first(true)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_properties_p);
    properties->loadProperties(false, true);
    // If 'properties->d_originalNumProps ==
    //          properties->d_propertises.size()',
    // then 'loadProperties' is no-op.
    // Otherwise, 'isNewStyleProperties' was 'true'.
}

// ACCESSORS
inline const bsl::string& MessagePropertiesIterator::name() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_properties_p);
    BSLS_ASSERT(d_iterator != d_properties_p->d_properties.end());

    return d_iterator->first;
}

inline bmqt::PropertyType::Enum MessagePropertiesIterator::type() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_properties_p);
    BSLS_ASSERT(d_iterator != d_properties_p->d_properties.end());

    return d_iterator->second.d_type;
}

inline bool MessagePropertiesIterator::getAsBool() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_properties_p);
    BSLS_ASSERT(d_iterator != d_properties_p->d_properties.end());

    return d_properties_p->getPropertyValue(d_iterator->second).the<bool>();
}

inline char MessagePropertiesIterator::getAsChar() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_properties_p);
    BSLS_ASSERT(d_iterator != d_properties_p->d_properties.end());

    return d_properties_p->getPropertyValue(d_iterator->second).the<char>();
}

inline short MessagePropertiesIterator::getAsShort() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_properties_p);
    BSLS_ASSERT(d_iterator != d_properties_p->d_properties.end());

    return d_properties_p->getPropertyValue(d_iterator->second).the<short>();
}

inline int MessagePropertiesIterator::getAsInt32() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_properties_p);
    BSLS_ASSERT(d_iterator != d_properties_p->d_properties.end());

    return d_properties_p->getPropertyValue(d_iterator->second).the<int>();
}

inline bsls::Types::Int64 MessagePropertiesIterator::getAsInt64() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_properties_p);
    BSLS_ASSERT(d_iterator != d_properties_p->d_properties.end());

    return d_properties_p->getPropertyValue(d_iterator->second)
        .the<bsls::Types::Int64>();
}

inline const bsl::string& MessagePropertiesIterator::getAsString() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_properties_p);
    BSLS_ASSERT(d_iterator != d_properties_p->d_properties.end());

    return d_properties_p->getPropertyValue(d_iterator->second)
        .the<bsl::string>();
}

inline const bsl::vector<char>& MessagePropertiesIterator::getAsBinary() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_properties_p);
    BSLS_ASSERT(d_iterator != d_properties_p->d_properties.end());

    return d_properties_p->getPropertyValue(d_iterator->second)
        .the<bsl::vector<char> >();
}

}  // close package namespace

// -----------------------
// class MessageProperties
// -----------------------
// FREE OPERATORS
inline bsl::ostream& bmqp::operator<<(bsl::ostream&                  stream,
                                      const bmqp::MessageProperties& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
