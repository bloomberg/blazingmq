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

// bmqa_messageproperties.h                                           -*-C++-*-
#ifndef INCLUDED_BMQA_MESSAGEPROPERTIES
#define INCLUDED_BMQA_MESSAGEPROPERTIES

/// @file bmqa_messageproperties.h
///
/// @brief Provide a VST representing message properties.
///
/// @bbref{bmqa::MessageProperties} provides a VST representing message
/// properties.  Message properties are a collection of name-value pairs that
/// producer can associate with a message, and consumer can retrieve from the
/// corresponding message.  In order to keep their usage flexible, no schema is
/// enforced for the message properties, and their format (names and data
/// types) should be negotiated by producers and consumers.  Message properties
/// can be used for routing, pipelining or filtering messages within the
/// application.  It can be efficient to specify such message attributes in the
/// properties instead of the message payload, because application does not
/// have to decode entire payload to retrieve these attributes.
/// @bbref{bmqa::MessagePropertiesIterator} provides a mechanism to iterate
/// over all the properties of a @bbref{bmqa::MessageProperties} object.
///
/// Restrictions on Property Names   {#bmqa_messageproperties_namerestrictions}
/// ==============================
///
///   - Length of a property name must be greater than zero and must *not*
///     exceed @bbref{bmqa::MessageProperties::k_MAX_PROPERTY_NAME_LENGTH}.
///
///   - First character of the property name must be alpha-numeric.
///
///   - Property names starting with the prefix "bmq." are reserved for
///     internal use and cannot be set by users.
///
/// Restrictions on Property Values {#bmqa_messageproperties_valuerestrictions}
/// ===============================
///
///   - Length of a property value must be non-negative (ie, can be zero) and
///     must *not* exceed
///     @bbref{bmqa::MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH}.  Note
///     that this restriction is obviously applicable to property values with
///     types @bbref{bmqt::PropertyType::e_STRING} and
///     @bbref{bmqt::PropertyType::e_BINARY}, because for all other property
///     value types, size is implicitly applicable based on the type (see @ref
///     bmqt_propertytype_types section in @bbref{bmqt::PropertyType}
///     component).
///
/// @see @bbref{bmqt::PropertyType}

// BMQ

#include <bmqt_propertytype.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_cstdint.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_alignedbuffer.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqp {
class MessageProperties;
}
namespace bmqp {
class MessagePropertiesIterator;
}

namespace bmqa {

// =======================
// class MessageProperties
// =======================

/// Provide a VST representing message properties.
class MessageProperties {
    // FRIENDS
    friend class MessagePropertiesIterator;

  private:
    // PRIVATE CONSTANTS

    /// Constant representing the maximum size of a
    /// `bmqp::MessageProperties` object, so that the below AlignedBuffer
    /// is big enough.
    static const int k_MAX_SIZEOF_BMQP_MESSAGEPROPERTIES = 184;

    // PRIVATE TYPES
    typedef bsls::AlignedBuffer<k_MAX_SIZEOF_BMQP_MESSAGEPROPERTIES>
        ImplBuffer;

  private:
    // DATA

    /// Pointer to the implementation object in `d_buffer`, providing a
    /// shortcut type safe cast to that object. This variable *must* *be* the
    /// first member of this class, as other components in bmqa package may
    /// reinterpret_cast to that variable.
    mutable bmqp::MessageProperties* d_impl_p;

    /// Buffer containing the implementation object, maximally aligned.
    ImplBuffer d_buffer;

    bslma::Allocator* d_allocator_p;

  public:
    // PUBLIC CONSTANTS

    /// Maximum number of properties that can appear in a `bmqa::Message`.
    static const int k_MAX_NUM_PROPERTIES = 255;

    /// Maximum length of all the properties (including their names, values
    /// and the wire protocol overhead).  Note that this value is just under
    /// 64 MB.
    static const int k_MAX_PROPERTIES_AREA_LENGTH = (64 * 1024 * 1024) - 8;

    /// Maximum length of a property name.
    static const int k_MAX_PROPERTY_NAME_LENGTH = 4095;

    /// ~64 MB
    /// Maximum length of a property value.  Note that this value is just under
    /// 64 MB.  Also note that this value is calculated assuming that there is
    /// only one property and property's name has maximum allowable length, and
    /// also takes into consideration the protocol overhead.
    static const int k_MAX_PROPERTY_VALUE_LENGTH = 67104745;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MessageProperties,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an empty instance of `MessageProperties`.  Optionally
    /// specify a `basicAllocator` used to supply memory.  Note that it is
    /// more efficient to use a `MessageProperties` object retrieved via
    /// `Session::loadMessageProperties`, instead of creating it via this
    /// constructor.
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
    /// type of the property.  Return false if the `name` property does not
    /// exist, and leave the `buffer` unchanged.
    bool remove(const bsl::string& name, bmqt::PropertyType::Enum* buffer = 0);

    /// Remove all properties from this instance.  Note that `numProperties`
    /// will return zero after invoking this method.
    void clear();

    int setPropertyAsBool(const bsl::string& name, bool value);
    int setPropertyAsChar(const bsl::string& name, char value);
    int setPropertyAsShort(const bsl::string& name, short value);
    int setPropertyAsInt32(const bsl::string& name, bsl::int32_t value);
    int setPropertyAsInt64(const bsl::string& name, bsls::Types::Int64 value);
    int setPropertyAsString(const bsl::string& name, const bsl::string& value);

    /// Set a property with the specified `name` having the specified
    /// `value` with the corresponding data type.  Return zero on success,
    /// and a non-zero value in case of failure.  Note that if a property
    /// with `name` and the same type exists, it will be updated with the
    /// provided `value`, however if the data type of the existing property
    /// differs, an error will be returned.
    int setPropertyAsBinary(const bsl::string&       name,
                            const bsl::vector<char>& value);

    /// Populate this instance with its BlazingMQ wire protocol
    /// representation from the specified `blob`.  Return zero on success,
    /// and a non-zero value otherwise.
    int streamIn(const bdlbb::Blob& blob);

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
    /// is undefined unless `hasProperty` returns true for the property
    /// `name`.
    bmqt::PropertyType::Enum propertyType(const bsl::string& name) const;

    bool               getPropertyAsBool(const bsl::string& name) const;
    char               getPropertyAsChar(const bsl::string& name) const;
    short              getPropertyAsShort(const bsl::string& name) const;
    bsl::int32_t       getPropertyAsInt32(const bsl::string& name) const;
    bsls::Types::Int64 getPropertyAsInt64(const bsl::string& name) const;
    const bsl::string& getPropertyAsString(const bsl::string& name) const;

    /// Return the property having the corresponding type and the specified
    /// `name`.  Behavior is undefined unless property with `name` exists.
    const bsl::vector<char>&
    getPropertyAsBinary(const bsl::string& name) const;

    bool  getPropertyAsBoolOr(const bsl::string& name, bool value) const;
    char  getPropertyAsCharOr(const bsl::string& name, char value) const;
    short getPropertyAsShortOr(const bsl::string& name, short value) const;
    bsl::int32_t       getPropertyAsInt32Or(const bsl::string& name,
                                            bsl::int32_t       value) const;
    bsls::Types::Int64 getPropertyAsInt64Or(const bsl::string& name,
                                            bsls::Types::Int64 value) const;
    const bsl::string& getPropertyAsStringOr(const bsl::string& name,
                                             const bsl::string& value) const;

    /// Return the property having the corresponding type and the specified
    /// `name` if property with such a name exists.  Return the specified
    /// `value` if property with `name` does not exist.
    const bsl::vector<char>&
    getPropertyAsBinaryOr(const bsl::string&       name,
                          const bsl::vector<char>& value) const;

    /// Return a blob having the BlazingMQ wire protocol representation of
    /// this instance, using the specified `bufferFactory` to build the
    /// blob.  Behavior is undefined unless `bufferFactory` is non-null.
    /// Note that if this instance is empty (i.e., `numProperties()` == 0),
    /// returned blob will be empty.  In other words, an empty instance has
    /// no wire representation.
    const bdlbb::Blob&
    streamOut(bdlbb::BlobBufferFactory* bufferFactory) const;

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
/// of `bmqa::MessageProperties`.  The order of the iteration is
/// implementation defined.  An iterator is *valid* if it is associated with
/// a property , otherwise it is *invalid*.  Behavior is undefined if the
/// underlying instance of `bmqa::MessageProperties` is modified during the
/// lifetime of this iterator.
class MessagePropertiesIterator {
  private:
    // PRIVATE CONSTANTS

    // Constant representing the maximum size of a
    // `bmqp::MessagePropertiesIterator` object, so that the below
    // AlignedBuffer is big enough.
    static const int k_MAX_SIZEOF_BMQP_MESSAGEPROPERTIESITER = 64;

    // PRIVATE TYPES
    typedef bsls::AlignedBuffer<k_MAX_SIZEOF_BMQP_MESSAGEPROPERTIESITER>
        ImplBuffer;

  private:
    // DATA
    mutable bmqp::MessagePropertiesIterator* d_impl_p;
    // Pointer to the implementation object
    // in 'd_buffer'.  This variable *must*
    // *be* the first member of this class.
    // If the value is null, the object
    // has not been constructed in the
    // 'd_buffer'.

    ImplBuffer d_buffer;
    // Buffer containing the implementation
    // object

  public:
    // CREATORS

    /// Create an empty iterator instance.  The only valid operations that
    /// can be invoked on an empty instance are copying, assignment and
    /// destruction.
    MessagePropertiesIterator();

    /// Create an iterator for the specified `properties`.  Behavior is
    /// undefined unless `properties` is not null.
    explicit MessagePropertiesIterator(const MessageProperties* properties);

    /// Copy constructor from the specified `other`.
    MessagePropertiesIterator(const MessagePropertiesIterator& other);

    /// Destroy this iterator.
    ~MessagePropertiesIterator();

    // MANIPULATORS

    /// Assignment operator from the specified `rhs`.
    MessagePropertiesIterator& operator=(const MessagePropertiesIterator& rhs);

    /// Advance this iterator to refer to the next property of the
    /// associated `MessageProperties` instance, if there is one and return
    /// true, return false otherwise.  Behavior is undefined unless this
    /// method is being invoked for the first time on this object or
    /// previous call to `hasNext` returned true.  Note that the order of
    /// the iteration is not specified.
    bool hasNext();

    // ACCESSORS

    /// Return a reference offering non-modifiable access to the name of the
    /// property being pointed by the iterator.  Behavior is undefined
    /// unless last call to `hasNext` returned true.
    const bsl::string& name() const;

    /// Return the data type of property being pointed by the iterator.
    /// Behavior is undefined unless last call to `hasNext` returned true;
    bmqt::PropertyType::Enum type() const;

    bool               getAsBool() const;
    char               getAsChar() const;
    short              getAsShort() const;
    bsl::int32_t       getAsInt32() const;
    bsls::Types::Int64 getAsInt64() const;
    const bsl::string& getAsString() const;

    /// Return property value having the corresponding type being currently
    /// being pointed by this iterator instance.  Behavior is undefined
    /// unless last call to `hasNext` returned true.  Behavior is also
    /// undefined unless property's data type matches the requested type.
    const bsl::vector<char>& getAsBinary() const;
};

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// class MessageProperties
// -----------------------

inline bsl::ostream& bmqa::operator<<(bsl::ostream&                  stream,
                                      const bmqa::MessageProperties& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
