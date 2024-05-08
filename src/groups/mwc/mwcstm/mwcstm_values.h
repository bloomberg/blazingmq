// Copyright 2024 Bloomberg Finance L.P.
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

// mwcstm_values.h              *DO NOT EDIT*              @generated -*-C++-*-
#ifndef INCLUDED_MWCSTM_VALUES
#define INCLUDED_MWCSTM_VALUES

//@PURPOSE: Provide value-semantic attribute classes

#include <bslalg_typetraits.h>

#include <bdlat_attributeinfo.h>

#include <bdlat_enumeratorinfo.h>

#include <bdlat_selectioninfo.h>

#include <bdlat_typetraits.h>

#include <bslh_hash.h>
#include <bsls_objectbuffer.h>

#include <bslma_default.h>

#include <bsls_assert.h>

#include <bdlb_nullablevalue.h>

#include <bsl_string.h>

#include <bsl_vector.h>

#include <bsls_types.h>

#include <bsl_iosfwd.h>
#include <bsl_limits.h>

#include <bsl_ostream.h>
#include <bsl_string.h>

namespace BloombergLP {

namespace bslma {
class Allocator;
}

namespace mwcstm {
class StatContextConfigurationChoice;
}
namespace mwcstm {
class StatValueUpdate;
}
namespace mwcstm {
class StatValueDefinition;
}
namespace mwcstm {
class StatContextConfiguration;
}
namespace mwcstm {
class StatContextUpdate;
}
namespace mwcstm {
class StatContextUpdateList;
}
namespace mwcstm {

// ====================================
// class StatContextConfigurationChoice
// ====================================

class StatContextConfigurationChoice {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<bsls::Types::Int64> d_id;
        bsls::ObjectBuffer<bsl::string>        d_name;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const StatContextConfigurationChoice& rhs) const;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED = -1,
        SELECTION_ID_ID        = 0,
        SELECTION_ID_NAME      = 1
    };

    enum { NUM_SELECTIONS = 2 };

    enum { SELECTION_INDEX_ID = 0, SELECTION_INDEX_NAME = 1 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_SelectionInfo SELECTION_INFO_ARRAY[];

    // CLASS METHODS
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    explicit StatContextConfigurationChoice(
        bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatContextConfigurationChoice' having the
    // default value.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

    StatContextConfigurationChoice(
        const StatContextConfigurationChoice& original,
        bslma::Allocator*                     basicAllocator = 0);
    // Create an object of type 'StatContextConfigurationChoice' having the
    // value of the specified 'original' object.  Use the optionally
    // specified 'basicAllocator' to supply memory.  If 'basicAllocator' is
    // 0, the currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatContextConfigurationChoice(
        StatContextConfigurationChoice&& original) noexcept;
    // Create an object of type 'StatContextConfigurationChoice' having the
    // value of the specified 'original' object.  After performing this
    // action, the 'original' object will be left in a valid, but
    // unspecified state.

    StatContextConfigurationChoice(StatContextConfigurationChoice&& original,
                                   bslma::Allocator* basicAllocator);
    // Create an object of type 'StatContextConfigurationChoice' having the
    // value of the specified 'original' object.  After performing this
    // action, the 'original' object will be left in a valid, but
    // unspecified state.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.
#endif

    ~StatContextConfigurationChoice();
    // Destroy this object.

    // MANIPULATORS
    StatContextConfigurationChoice&
    operator=(const StatContextConfigurationChoice& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatContextConfigurationChoice&
    operator=(StatContextConfigurationChoice&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

    bsls::Types::Int64& makeId();
    bsls::Types::Int64& makeId(bsls::Types::Int64 value);
    // Set the value of this object to be a "Id" value.  Optionally specify
    // the 'value' of the "Id".  If 'value' is not specified, the default
    // "Id" value is used.

    bsl::string& makeName();
    bsl::string& makeName(const bsl::string& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    bsl::string& makeName(bsl::string&& value);
#endif
    // Set the value of this object to be a "Name" value.  Optionally
    // specify the 'value' of the "Name".  If 'value' is not specified, the
    // default "Name" value is used.

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    bsls::Types::Int64& id();
    // Return a reference to the modifiable "Id" selection of this object
    // if "Id" is the current selection.  The behavior is undefined unless
    // "Id" is the selection of this object.

    bsl::string& name();
    // Return a reference to the modifiable "Name" selection of this object
    // if "Name" is the current selection.  The behavior is undefined
    // unless "Name" is the selection of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const bsls::Types::Int64& id() const;
    // Return a reference to the non-modifiable "Id" selection of this
    // object if "Id" is the current selection.  The behavior is undefined
    // unless "Id" is the selection of this object.

    const bsl::string& name() const;
    // Return a reference to the non-modifiable "Name" selection of this
    // object if "Name" is the current selection.  The behavior is
    // undefined unless "Name" is the selection of this object.

    bool isIdValue() const;
    // Return 'true' if the value of this object is a "Id" value, and
    // return 'false' otherwise.

    bool isNameValue() const;
    // Return 'true' if the value of this object is a "Name" value, and
    // return 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StatContextConfigurationChoice& lhs,
                           const StatContextConfigurationChoice& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'StatContextConfigurationChoice'
    // objects have the same value if either the selections in both objects
    // have the same ids and the same values, or both selections are
    // undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const StatContextConfigurationChoice& lhs,
                           const StatContextConfigurationChoice& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream,
                                    const StatContextConfigurationChoice& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&                     hashAlg,
                           const StatContextConfigurationChoice& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StatContextConfigurationChoice'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatContextConfigurationChoice)

namespace mwcstm {

// ===================================
// class StatContextConfigurationFlags
// ===================================

struct StatContextConfigurationFlags {
    // This type enumerates the flags that can be set on a
    // 'StatContextConfiguration'.  A configuration having the 'IS_TABLE' flag
    // denotes that it is a table context, and 'STORE_EXPIRED_VALUES' indicates
    // it should remember the values of expired subcontexts.

  public:
    // TYPES
    enum Value { E_IS_TABLE = 0, E_STORE_EXPIRED_VALUES = 1 };

    enum { NUM_ENUMERATORS = 2 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    static const char* toString(Value value);
    // Return the string representation exactly matching the enumerator
    // name corresponding to the specified enumeration 'value'.

    static int fromString(Value* result, const char* string, int stringLength);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string' of the specified 'stringLength'.  Return 0 on
    // success, and a non-zero value with no effect on 'result' otherwise
    // (i.e., 'string' does not match any enumerator).

    static int fromString(Value* result, const bsl::string& string);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'string' does not match any
    // enumerator).

    static int fromInt(Value* result, int number);
    // Load into the specified 'result' the enumerator matching the
    // specified 'number'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'number' does not match any
    // enumerator).

    static bsl::ostream& print(bsl::ostream& stream, Value value);
    // Write to the specified 'stream' the string representation of
    // the specified enumeration 'value'.  Return a reference to
    // the modifiable 'stream'.

    // HIDDEN FRIENDS
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return StatContextConfigurationFlags::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mwcstm::StatContextConfigurationFlags)

namespace mwcstm {

// ============================
// class StatContextUpdateFlags
// ============================

struct StatContextUpdateFlags {
    // This type enumerates the flags that can be set on a 'StatContext'.  A
    // context having the 'CONTEXT_CREATED' flag indicates that the context was
    // just created, and the 'CONTEXT_DELETED' flag indicates that it was
    // deleted.

  public:
    // TYPES
    enum Value { E_CONTEXT_CREATED = 0, E_CONTEXT_DELETED = 1 };

    enum { NUM_ENUMERATORS = 2 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    static const char* toString(Value value);
    // Return the string representation exactly matching the enumerator
    // name corresponding to the specified enumeration 'value'.

    static int fromString(Value* result, const char* string, int stringLength);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string' of the specified 'stringLength'.  Return 0 on
    // success, and a non-zero value with no effect on 'result' otherwise
    // (i.e., 'string' does not match any enumerator).

    static int fromString(Value* result, const bsl::string& string);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'string' does not match any
    // enumerator).

    static int fromInt(Value* result, int number);
    // Load into the specified 'result' the enumerator matching the
    // specified 'number'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'number' does not match any
    // enumerator).

    static bsl::ostream& print(bsl::ostream& stream, Value value);
    // Write to the specified 'stream' the string representation of
    // the specified enumeration 'value'.  Return a reference to
    // the modifiable 'stream'.

    // HIDDEN FRIENDS
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return StatContextUpdateFlags::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mwcstm::StatContextUpdateFlags)

namespace mwcstm {

// =====================
// class StatValueFields
// =====================

struct StatValueFields {
    // This type enumerates the different bit indices corresponding to fields
    // in a stat value.

  public:
    // TYPES
    enum Value {
        E_ABSOLUTE_MIN = 0,
        E_ABSOLUTE_MAX = 1,
        E_MIN          = 2,
        E_MAX          = 3,
        E_EVENTS       = 4,
        E_SUM          = 5,
        E_VALUE        = 6,
        E_INCREMENTS   = 7,
        E_DECREMENTS   = 8
    };

    enum { NUM_ENUMERATORS = 9 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    static const char* toString(Value value);
    // Return the string representation exactly matching the enumerator
    // name corresponding to the specified enumeration 'value'.

    static int fromString(Value* result, const char* string, int stringLength);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string' of the specified 'stringLength'.  Return 0 on
    // success, and a non-zero value with no effect on 'result' otherwise
    // (i.e., 'string' does not match any enumerator).

    static int fromString(Value* result, const bsl::string& string);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'string' does not match any
    // enumerator).

    static int fromInt(Value* result, int number);
    // Load into the specified 'result' the enumerator matching the
    // specified 'number'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'number' does not match any
    // enumerator).

    static bsl::ostream& print(bsl::ostream& stream, Value value);
    // Write to the specified 'stream' the string representation of
    // the specified enumeration 'value'.  Return a reference to
    // the modifiable 'stream'.

    // HIDDEN FRIENDS
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return StatValueFields::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mwcstm::StatValueFields)

namespace mwcstm {

// ===================
// class StatValueType
// ===================

struct StatValueType {
    // This type enumerates the different types of stat values, specifically
    // 'CONTINUOUS' and 'DISCRETE' values.

  public:
    // TYPES
    enum Value { E_CONTINUOUS = 0, E_DISCRETE = 1 };

    enum { NUM_ENUMERATORS = 2 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    static const char* toString(Value value);
    // Return the string representation exactly matching the enumerator
    // name corresponding to the specified enumeration 'value'.

    static int fromString(Value* result, const char* string, int stringLength);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string' of the specified 'stringLength'.  Return 0 on
    // success, and a non-zero value with no effect on 'result' otherwise
    // (i.e., 'string' does not match any enumerator).

    static int fromString(Value* result, const bsl::string& string);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'string' does not match any
    // enumerator).

    static int fromInt(Value* result, int number);
    // Load into the specified 'result' the enumerator matching the
    // specified 'number'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'number' does not match any
    // enumerator).

    static bsl::ostream& print(bsl::ostream& stream, Value value);
    // Write to the specified 'stream' the string representation of
    // the specified enumeration 'value'.  Return a reference to
    // the modifiable 'stream'.

    // HIDDEN FRIENDS
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return StatValueType::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mwcstm::StatValueType)

namespace mwcstm {

// =====================
// class StatValueUpdate
// =====================

class StatValueUpdate {
    // This type represents changes made to a particular state value.  The bits
    // of the 'fieldMask' indicate which actual fields in a stat value the
    // elements of 'fields' are updating.  For instance, if the value in
    // question is a continuous value, and 'fieldMask' is 0b001100, then
    // 'fields' should contain two elements, with 'fields[0]' being the update
    // to the 'min' value, and 'fields[1]' being the update to the 'max' value.

    // INSTANCE DATA
    bsl::vector<bsls::Types::Int64> d_fields;
    unsigned int                    d_fieldMask;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_FIELD_MASK = 0, ATTRIBUTE_ID_FIELDS = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_FIELD_MASK = 0, ATTRIBUTE_INDEX_FIELDS = 1 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit StatValueUpdate(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatValueUpdate' having the default value.
    //  Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    StatValueUpdate(const StatValueUpdate& original,
                    bslma::Allocator*      basicAllocator = 0);
    // Create an object of type 'StatValueUpdate' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatValueUpdate(StatValueUpdate&& original) noexcept;
    // Create an object of type 'StatValueUpdate' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    StatValueUpdate(StatValueUpdate&& original,
                    bslma::Allocator* basicAllocator);
    // Create an object of type 'StatValueUpdate' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~StatValueUpdate();
    // Destroy this object.

    // MANIPULATORS
    StatValueUpdate& operator=(const StatValueUpdate& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatValueUpdate& operator=(StatValueUpdate&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    unsigned int& fieldMask();
    // Return a reference to the modifiable "FieldMask" attribute of this
    // object.

    bsl::vector<bsls::Types::Int64>& fields();
    // Return a reference to the modifiable "Fields" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    unsigned int fieldMask() const;
    // Return the value of the "FieldMask" attribute of this object.

    const bsl::vector<bsls::Types::Int64>& fields() const;
    // Return a reference offering non-modifiable access to the "Fields"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StatValueUpdate& lhs,
                           const StatValueUpdate& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.fieldMask() == rhs.fieldMask() &&
               lhs.fields() == rhs.fields();
    }

    friend bool operator!=(const StatValueUpdate& lhs,
                           const StatValueUpdate& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&          stream,
                                    const StatValueUpdate& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&      hashAlg,
                           const StatValueUpdate& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StatValueUpdate'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.fieldMask());
        hashAppend(hashAlg, object.fields());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatValueUpdate)

namespace mwcstm {

// =========================
// class StatValueDefinition
// =========================

class StatValueDefinition {
    // This type represents the definition of a stat collecting value, having a
    // 'name', a 'type' and an array of 'historySizes' indicating the number of
    // snapshots and levels of snapshots to keep.

    // INSTANCE DATA
    bsl::vector<unsigned int> d_historySizes;
    bsl::string               d_name;
    StatValueType::Value      d_type;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_NAME          = 0,
        ATTRIBUTE_ID_TYPE          = 1,
        ATTRIBUTE_ID_HISTORY_SIZES = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_NAME          = 0,
        ATTRIBUTE_INDEX_TYPE          = 1,
        ATTRIBUTE_INDEX_HISTORY_SIZES = 2
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit StatValueDefinition(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatValueDefinition' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    StatValueDefinition(const StatValueDefinition& original,
                        bslma::Allocator*          basicAllocator = 0);
    // Create an object of type 'StatValueDefinition' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatValueDefinition(StatValueDefinition&& original) noexcept;
    // Create an object of type 'StatValueDefinition' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    StatValueDefinition(StatValueDefinition&& original,
                        bslma::Allocator*     basicAllocator);
    // Create an object of type 'StatValueDefinition' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~StatValueDefinition();
    // Destroy this object.

    // MANIPULATORS
    StatValueDefinition& operator=(const StatValueDefinition& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatValueDefinition& operator=(StatValueDefinition&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& name();
    // Return a reference to the modifiable "Name" attribute of this
    // object.

    StatValueType::Value& type();
    // Return a reference to the modifiable "Type" attribute of this
    // object.

    bsl::vector<unsigned int>& historySizes();
    // Return a reference to the modifiable "HistorySizes" attribute of
    // this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& name() const;
    // Return a reference offering non-modifiable access to the "Name"
    // attribute of this object.

    StatValueType::Value type() const;
    // Return the value of the "Type" attribute of this object.

    const bsl::vector<unsigned int>& historySizes() const;
    // Return a reference offering non-modifiable access to the
    // "HistorySizes" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StatValueDefinition& lhs,
                           const StatValueDefinition& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.name() == rhs.name() && lhs.type() == rhs.type() &&
               lhs.historySizes() == rhs.historySizes();
    }

    friend bool operator!=(const StatValueDefinition& lhs,
                           const StatValueDefinition& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&              stream,
                                    const StatValueDefinition& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&          hashAlg,
                           const StatValueDefinition& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StatValueDefinition'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatValueDefinition)

namespace mwcstm {

// ==============================
// class StatContextConfiguration
// ==============================

class StatContextConfiguration {
    // This type represents the configuration of a stat context, having some
    // configuration 'flags', a user supplied 'userId' that may either be a
    // long integer 'id' or a string 'name', and a set of 'values' to track.
    // Note that 'values' may be omitted if this configuration is for a
    // subcontext of a table context.

    // INSTANCE DATA
    bsl::vector<StatValueDefinition> d_values;
    StatContextConfigurationChoice   d_choice;
    unsigned int                     d_flags;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_FLAGS  = 0,
        ATTRIBUTE_ID_CHOICE = 1,
        ATTRIBUTE_ID_VALUES = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_FLAGS  = 0,
        ATTRIBUTE_INDEX_CHOICE = 1,
        ATTRIBUTE_INDEX_VALUES = 2
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit StatContextConfiguration(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatContextConfiguration' having the
    // default value.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

    StatContextConfiguration(const StatContextConfiguration& original,
                             bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatContextConfiguration' having the value
    // of the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatContextConfiguration(StatContextConfiguration&& original) noexcept;
    // Create an object of type 'StatContextConfiguration' having the value
    // of the specified 'original' object.  After performing this action,
    // the 'original' object will be left in a valid, but unspecified
    // state.

    StatContextConfiguration(StatContextConfiguration&& original,
                             bslma::Allocator*          basicAllocator);
    // Create an object of type 'StatContextConfiguration' having the value
    // of the specified 'original' object.  After performing this action,
    // the 'original' object will be left in a valid, but unspecified
    // state.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.
#endif

    ~StatContextConfiguration();
    // Destroy this object.

    // MANIPULATORS
    StatContextConfiguration& operator=(const StatContextConfiguration& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatContextConfiguration& operator=(StatContextConfiguration&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    unsigned int& flags();
    // Return a reference to the modifiable "Flags" attribute of this
    // object.

    StatContextConfigurationChoice& choice();
    // Return a reference to the modifiable "Choice" attribute of this
    // object.

    bsl::vector<StatValueDefinition>& values();
    // Return a reference to the modifiable "Values" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    unsigned int flags() const;
    // Return the value of the "Flags" attribute of this object.

    const StatContextConfigurationChoice& choice() const;
    // Return a reference offering non-modifiable access to the "Choice"
    // attribute of this object.

    const bsl::vector<StatValueDefinition>& values() const;
    // Return a reference offering non-modifiable access to the "Values"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StatContextConfiguration& lhs,
                           const StatContextConfiguration& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.flags() == rhs.flags() && lhs.choice() == rhs.choice() &&
               lhs.values() == rhs.values();
    }

    friend bool operator!=(const StatContextConfiguration& lhs,
                           const StatContextConfiguration& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                   stream,
                                    const StatContextConfiguration& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&               hashAlg,
                           const StatContextConfiguration& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StatContextConfiguration'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatContextConfiguration)

namespace mwcstm {

// =======================
// class StatContextUpdate
// =======================

class StatContextUpdate {
    // This type represents an externalizable 'StatContext', having an integer
    // 'id' distinct from all sibling contexts, a set of 'flags' denoting if
    // this is a new or deleted context, an optional 'configuration' that
    // describes the context structure that is excluded if redundant, a set of
    // 'directValues' and 'expiredValues', and a set of 'subcontexts'.

    // INSTANCE DATA
    bsls::Types::Int64                            d_timeStamp;
    bsl::vector<StatValueUpdate>                  d_directValues;
    bsl::vector<StatValueUpdate>                  d_expiredValues;
    bsl::vector<StatContextUpdate>                d_subcontexts;
    bdlb::NullableValue<StatContextConfiguration> d_configuration;
    unsigned int                                  d_flags;
    int                                           d_id;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const StatContextUpdate& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_ID             = 0,
        ATTRIBUTE_ID_FLAGS          = 1,
        ATTRIBUTE_ID_TIME_STAMP     = 2,
        ATTRIBUTE_ID_CONFIGURATION  = 3,
        ATTRIBUTE_ID_DIRECT_VALUES  = 4,
        ATTRIBUTE_ID_EXPIRED_VALUES = 5,
        ATTRIBUTE_ID_SUBCONTEXTS    = 6
    };

    enum { NUM_ATTRIBUTES = 7 };

    enum {
        ATTRIBUTE_INDEX_ID             = 0,
        ATTRIBUTE_INDEX_FLAGS          = 1,
        ATTRIBUTE_INDEX_TIME_STAMP     = 2,
        ATTRIBUTE_INDEX_CONFIGURATION  = 3,
        ATTRIBUTE_INDEX_DIRECT_VALUES  = 4,
        ATTRIBUTE_INDEX_EXPIRED_VALUES = 5,
        ATTRIBUTE_INDEX_SUBCONTEXTS    = 6
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit StatContextUpdate(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatContextUpdate' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    StatContextUpdate(const StatContextUpdate& original,
                      bslma::Allocator*        basicAllocator = 0);
    // Create an object of type 'StatContextUpdate' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatContextUpdate(StatContextUpdate&& original) noexcept;
    // Create an object of type 'StatContextUpdate' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    StatContextUpdate(StatContextUpdate&& original,
                      bslma::Allocator*   basicAllocator);
    // Create an object of type 'StatContextUpdate' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~StatContextUpdate();
    // Destroy this object.

    // MANIPULATORS
    StatContextUpdate& operator=(const StatContextUpdate& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatContextUpdate& operator=(StatContextUpdate&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    int& id();
    // Return a reference to the modifiable "Id" attribute of this object.

    unsigned int& flags();
    // Return a reference to the modifiable "Flags" attribute of this
    // object.

    bsls::Types::Int64& timeStamp();
    // Return a reference to the modifiable "TimeStamp" attribute of this
    // object.

    bdlb::NullableValue<StatContextConfiguration>& configuration();
    // Return a reference to the modifiable "Configuration" attribute of
    // this object.

    bsl::vector<StatValueUpdate>& directValues();
    // Return a reference to the modifiable "DirectValues" attribute of
    // this object.

    bsl::vector<StatValueUpdate>& expiredValues();
    // Return a reference to the modifiable "ExpiredValues" attribute of
    // this object.

    bsl::vector<StatContextUpdate>& subcontexts();
    // Return a reference to the modifiable "Subcontexts" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    int id() const;
    // Return the value of the "Id" attribute of this object.

    unsigned int flags() const;
    // Return the value of the "Flags" attribute of this object.

    bsls::Types::Int64 timeStamp() const;
    // Return the value of the "TimeStamp" attribute of this object.

    const bdlb::NullableValue<StatContextConfiguration>& configuration() const;
    // Return a reference offering non-modifiable access to the
    // "Configuration" attribute of this object.

    const bsl::vector<StatValueUpdate>& directValues() const;
    // Return a reference offering non-modifiable access to the
    // "DirectValues" attribute of this object.

    const bsl::vector<StatValueUpdate>& expiredValues() const;
    // Return a reference offering non-modifiable access to the
    // "ExpiredValues" attribute of this object.

    const bsl::vector<StatContextUpdate>& subcontexts() const;
    // Return a reference offering non-modifiable access to the
    // "Subcontexts" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StatContextUpdate& lhs,
                           const StatContextUpdate& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const StatContextUpdate& lhs,
                           const StatContextUpdate& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const StatContextUpdate& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const StatContextUpdate& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StatContextUpdate'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatContextUpdate)

namespace mwcstm {

// ===========================
// class StatContextUpdateList
// ===========================

class StatContextUpdateList {
    // This type represents a sequence of 'StatContext' objects.

    // INSTANCE DATA
    bsl::vector<StatContextUpdate> d_contexts;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_CONTEXTS = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_CONTEXTS = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit StatContextUpdateList(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'StatContextUpdateList' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    StatContextUpdateList(const StatContextUpdateList& original,
                          bslma::Allocator*            basicAllocator = 0);
    // Create an object of type 'StatContextUpdateList' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatContextUpdateList(StatContextUpdateList&& original) noexcept;
    // Create an object of type 'StatContextUpdateList' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    StatContextUpdateList(StatContextUpdateList&& original,
                          bslma::Allocator*       basicAllocator);
    // Create an object of type 'StatContextUpdateList' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~StatContextUpdateList();
    // Destroy this object.

    // MANIPULATORS
    StatContextUpdateList& operator=(const StatContextUpdateList& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StatContextUpdateList& operator=(StatContextUpdateList&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon
    // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' sequentially on the address of
    // each (modifiable) attribute of this object, supplying 'manipulator'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'manipulator' (i.e., the invocation that
    // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'id',
    // supplying 'manipulator' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if 'id' identifies an attribute of this
    // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::vector<StatContextUpdate>& contexts();
    // Return a reference to the modifiable "Contexts" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
    // Format this object to the specified output 'stream' at the
    // optionally specified indentation 'level' and return a reference to
    // the modifiable 'stream'.  If 'level' is specified, optionally
    // specify 'spacesPerLevel', the number of spaces per indentation level
    // for this and all of its nested objects.  Each line is indented by
    // the absolute value of 'level * spacesPerLevel'.  If 'level' is
    // negative, suppress indentation of the first line.  If
    // 'spacesPerLevel' is negative, suppress line breaks and format the
    // entire output on one line.  If 'stream' is initially invalid, this
    // operation has no effect.  Note that a trailing newline is provided
    // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' sequentially on each
    // (non-modifiable) attribute of this object, supplying 'accessor'
    // with the corresponding attribute information structure until such
    // invocation returns a non-zero value.  Return the value from the
    // last invocation of 'accessor' (i.e., the invocation that terminated
    // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'id', supplying 'accessor'
    // with the corresponding attribute information structure.  Return the
    // value returned from the invocation of 'accessor' if 'id' identifies
    // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::vector<StatContextUpdate>& contexts() const;
    // Return a reference offering non-modifiable access to the "Contexts"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StatContextUpdateList& lhs,
                           const StatContextUpdateList& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.contexts() == rhs.contexts();
    }

    friend bool operator!=(const StatContextUpdateList& lhs,
                           const StatContextUpdateList& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                stream,
                                    const StatContextUpdateList& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                           const StatContextUpdateList& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StatContextUpdateList'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.contexts());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatContextUpdateList)

//=============================================================================
//                          INLINE DEFINITIONS
//=============================================================================

namespace mwcstm {

// ------------------------------------
// class StatContextConfigurationChoice
// ------------------------------------

// CLASS METHODS
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void StatContextConfigurationChoice::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef StatContextConfigurationChoice Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_ID: hashAppend(hashAlgorithm, this->id()); break;
    case Class::SELECTION_ID_NAME:
        hashAppend(hashAlgorithm, this->name());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool StatContextConfigurationChoice::isEqualTo(
    const StatContextConfigurationChoice& rhs) const
{
    typedef StatContextConfigurationChoice Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_ID: return this->id() == rhs.id();
        case Class::SELECTION_ID_NAME: return this->name() == rhs.name();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

// CREATORS
inline StatContextConfigurationChoice::StatContextConfigurationChoice(
    bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline StatContextConfigurationChoice::~StatContextConfigurationChoice()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int StatContextConfigurationChoice::manipulateSelection(
    t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case StatContextConfigurationChoice::SELECTION_ID_ID:
        return manipulator(&d_id.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_ID]);
    case StatContextConfigurationChoice::SELECTION_ID_NAME:
        return manipulator(&d_name.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_NAME]);
    default:
        BSLS_ASSERT(StatContextConfigurationChoice::SELECTION_ID_UNDEFINED ==
                    d_selectionId);
        return -1;
    }
}

inline bsls::Types::Int64& StatContextConfigurationChoice::id()
{
    BSLS_ASSERT(SELECTION_ID_ID == d_selectionId);
    return d_id.object();
}

inline bsl::string& StatContextConfigurationChoice::name()
{
    BSLS_ASSERT(SELECTION_ID_NAME == d_selectionId);
    return d_name.object();
}

// ACCESSORS
inline int StatContextConfigurationChoice::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int StatContextConfigurationChoice::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_ID:
        return accessor(d_id.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_ID]);
    case SELECTION_ID_NAME:
        return accessor(d_name.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_NAME]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const bsls::Types::Int64& StatContextConfigurationChoice::id() const
{
    BSLS_ASSERT(SELECTION_ID_ID == d_selectionId);
    return d_id.object();
}

inline const bsl::string& StatContextConfigurationChoice::name() const
{
    BSLS_ASSERT(SELECTION_ID_NAME == d_selectionId);
    return d_name.object();
}

inline bool StatContextConfigurationChoice::isIdValue() const
{
    return SELECTION_ID_ID == d_selectionId;
}

inline bool StatContextConfigurationChoice::isNameValue() const
{
    return SELECTION_ID_NAME == d_selectionId;
}

inline bool StatContextConfigurationChoice::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}

// -----------------------------------
// class StatContextConfigurationFlags
// -----------------------------------

// CLASS METHODS
inline int StatContextConfigurationFlags::fromString(Value*             result,
                                                     const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream& StatContextConfigurationFlags::print(
    bsl::ostream&                        stream,
    StatContextConfigurationFlags::Value value)
{
    return stream << toString(value);
}

// ----------------------------
// class StatContextUpdateFlags
// ----------------------------

// CLASS METHODS
inline int StatContextUpdateFlags::fromString(Value*             result,
                                              const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream&
StatContextUpdateFlags::print(bsl::ostream&                 stream,
                              StatContextUpdateFlags::Value value)
{
    return stream << toString(value);
}

// ---------------------
// class StatValueFields
// ---------------------

// CLASS METHODS
inline int StatValueFields::fromString(Value*             result,
                                       const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream& StatValueFields::print(bsl::ostream&          stream,
                                            StatValueFields::Value value)
{
    return stream << toString(value);
}

// -------------------
// class StatValueType
// -------------------

// CLASS METHODS
inline int StatValueType::fromString(Value* result, const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream& StatValueType::print(bsl::ostream&        stream,
                                          StatValueType::Value value)
{
    return stream << toString(value);
}

// ---------------------
// class StatValueUpdate
// ---------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StatValueUpdate::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_fieldMask,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FIELD_MASK]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_fields, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FIELDS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StatValueUpdate::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_FIELD_MASK: {
        return manipulator(&d_fieldMask,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FIELD_MASK]);
    }
    case ATTRIBUTE_ID_FIELDS: {
        return manipulator(&d_fields,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FIELDS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StatValueUpdate::manipulateAttribute(t_MANIPULATOR& manipulator,
                                         const char*    name,
                                         int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline unsigned int& StatValueUpdate::fieldMask()
{
    return d_fieldMask;
}

inline bsl::vector<bsls::Types::Int64>& StatValueUpdate::fields()
{
    return d_fields;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StatValueUpdate::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_fieldMask,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FIELD_MASK]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_fields, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FIELDS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StatValueUpdate::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_FIELD_MASK: {
        return accessor(d_fieldMask,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FIELD_MASK]);
    }
    case ATTRIBUTE_ID_FIELDS: {
        return accessor(d_fields,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FIELDS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StatValueUpdate::accessAttribute(t_ACCESSOR& accessor,
                                     const char* name,
                                     int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline unsigned int StatValueUpdate::fieldMask() const
{
    return d_fieldMask;
}

inline const bsl::vector<bsls::Types::Int64>& StatValueUpdate::fields() const
{
    return d_fields;
}

// -------------------------
// class StatValueDefinition
// -------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void StatValueDefinition::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->name());
    hashAppend(hashAlgorithm, this->type());
    hashAppend(hashAlgorithm, this->historySizes());
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StatValueDefinition::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_type, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TYPE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_historySizes,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HISTORY_SIZES]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StatValueDefinition::manipulateAttribute(t_MANIPULATOR& manipulator,
                                             int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_TYPE: {
        return manipulator(&d_type,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TYPE]);
    }
    case ATTRIBUTE_ID_HISTORY_SIZES: {
        return manipulator(
            &d_historySizes,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HISTORY_SIZES]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StatValueDefinition::manipulateAttribute(t_MANIPULATOR& manipulator,
                                             const char*    name,
                                             int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& StatValueDefinition::name()
{
    return d_name;
}

inline StatValueType::Value& StatValueDefinition::type()
{
    return d_type;
}

inline bsl::vector<unsigned int>& StatValueDefinition::historySizes()
{
    return d_historySizes;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StatValueDefinition::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_type, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TYPE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_historySizes,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HISTORY_SIZES]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StatValueDefinition::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_TYPE: {
        return accessor(d_type, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TYPE]);
    }
    case ATTRIBUTE_ID_HISTORY_SIZES: {
        return accessor(d_historySizes,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HISTORY_SIZES]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StatValueDefinition::accessAttribute(t_ACCESSOR& accessor,
                                         const char* name,
                                         int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& StatValueDefinition::name() const
{
    return d_name;
}

inline StatValueType::Value StatValueDefinition::type() const
{
    return d_type;
}

inline const bsl::vector<unsigned int>&
StatValueDefinition::historySizes() const
{
    return d_historySizes;
}

// ------------------------------
// class StatContextConfiguration
// ------------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void StatContextConfiguration::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->flags());
    hashAppend(hashAlgorithm, this->choice());
    hashAppend(hashAlgorithm, this->values());
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StatContextConfiguration::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_flags, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_choice, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_values, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUES]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StatContextConfiguration::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                  int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_FLAGS: {
        return manipulator(&d_flags,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    }
    case ATTRIBUTE_ID_CHOICE: {
        return manipulator(&d_choice,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    }
    case ATTRIBUTE_ID_VALUES: {
        return manipulator(&d_values,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUES]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StatContextConfiguration::manipulateAttribute(t_MANIPULATOR& manipulator,
                                                  const char*    name,
                                                  int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline unsigned int& StatContextConfiguration::flags()
{
    return d_flags;
}

inline StatContextConfigurationChoice& StatContextConfiguration::choice()
{
    return d_choice;
}

inline bsl::vector<StatValueDefinition>& StatContextConfiguration::values()
{
    return d_values;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StatContextConfiguration::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_flags, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_choice, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_values, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUES]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StatContextConfiguration::accessAttribute(t_ACCESSOR& accessor,
                                              int         id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_FLAGS: {
        return accessor(d_flags, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    }
    case ATTRIBUTE_ID_CHOICE: {
        return accessor(d_choice,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    }
    case ATTRIBUTE_ID_VALUES: {
        return accessor(d_values,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUES]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StatContextConfiguration::accessAttribute(t_ACCESSOR& accessor,
                                              const char* name,
                                              int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline unsigned int StatContextConfiguration::flags() const
{
    return d_flags;
}

inline const StatContextConfigurationChoice&
StatContextConfiguration::choice() const
{
    return d_choice;
}

inline const bsl::vector<StatValueDefinition>&
StatContextConfiguration::values() const
{
    return d_values;
}

// -----------------------
// class StatContextUpdate
// -----------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void StatContextUpdate::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->id());
    hashAppend(hashAlgorithm, this->flags());
    hashAppend(hashAlgorithm, this->timeStamp());
    hashAppend(hashAlgorithm, this->configuration());
    hashAppend(hashAlgorithm, this->directValues());
    hashAppend(hashAlgorithm, this->expiredValues());
    hashAppend(hashAlgorithm, this->subcontexts());
}

inline bool StatContextUpdate::isEqualTo(const StatContextUpdate& rhs) const
{
    return this->id() == rhs.id() && this->flags() == rhs.flags() &&
           this->timeStamp() == rhs.timeStamp() &&
           this->configuration() == rhs.configuration() &&
           this->directValues() == rhs.directValues() &&
           this->expiredValues() == rhs.expiredValues() &&
           this->subcontexts() == rhs.subcontexts();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StatContextUpdate::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_flags, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_timeStamp,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIME_STAMP]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_configuration,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURATION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_directValues,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DIRECT_VALUES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_expiredValues,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPIRED_VALUES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_subcontexts,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBCONTEXTS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StatContextUpdate::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ID: {
        return manipulator(&d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    }
    case ATTRIBUTE_ID_FLAGS: {
        return manipulator(&d_flags,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    }
    case ATTRIBUTE_ID_TIME_STAMP: {
        return manipulator(&d_timeStamp,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIME_STAMP]);
    }
    case ATTRIBUTE_ID_CONFIGURATION: {
        return manipulator(
            &d_configuration,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURATION]);
    }
    case ATTRIBUTE_ID_DIRECT_VALUES: {
        return manipulator(
            &d_directValues,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DIRECT_VALUES]);
    }
    case ATTRIBUTE_ID_EXPIRED_VALUES: {
        return manipulator(
            &d_expiredValues,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPIRED_VALUES]);
    }
    case ATTRIBUTE_ID_SUBCONTEXTS: {
        return manipulator(&d_subcontexts,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBCONTEXTS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StatContextUpdate::manipulateAttribute(t_MANIPULATOR& manipulator,
                                           const char*    name,
                                           int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline int& StatContextUpdate::id()
{
    return d_id;
}

inline unsigned int& StatContextUpdate::flags()
{
    return d_flags;
}

inline bsls::Types::Int64& StatContextUpdate::timeStamp()
{
    return d_timeStamp;
}

inline bdlb::NullableValue<StatContextConfiguration>&
StatContextUpdate::configuration()
{
    return d_configuration;
}

inline bsl::vector<StatValueUpdate>& StatContextUpdate::directValues()
{
    return d_directValues;
}

inline bsl::vector<StatValueUpdate>& StatContextUpdate::expiredValues()
{
    return d_expiredValues;
}

inline bsl::vector<StatContextUpdate>& StatContextUpdate::subcontexts()
{
    return d_subcontexts;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StatContextUpdate::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_flags, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_timeStamp,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIME_STAMP]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_configuration,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURATION]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_directValues,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DIRECT_VALUES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_expiredValues,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPIRED_VALUES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_subcontexts,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBCONTEXTS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StatContextUpdate::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ID: {
        return accessor(d_id, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID]);
    }
    case ATTRIBUTE_ID_FLAGS: {
        return accessor(d_flags, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    }
    case ATTRIBUTE_ID_TIME_STAMP: {
        return accessor(d_timeStamp,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIME_STAMP]);
    }
    case ATTRIBUTE_ID_CONFIGURATION: {
        return accessor(d_configuration,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURATION]);
    }
    case ATTRIBUTE_ID_DIRECT_VALUES: {
        return accessor(d_directValues,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DIRECT_VALUES]);
    }
    case ATTRIBUTE_ID_EXPIRED_VALUES: {
        return accessor(d_expiredValues,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPIRED_VALUES]);
    }
    case ATTRIBUTE_ID_SUBCONTEXTS: {
        return accessor(d_subcontexts,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBCONTEXTS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StatContextUpdate::accessAttribute(t_ACCESSOR& accessor,
                                       const char* name,
                                       int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline int StatContextUpdate::id() const
{
    return d_id;
}

inline unsigned int StatContextUpdate::flags() const
{
    return d_flags;
}

inline bsls::Types::Int64 StatContextUpdate::timeStamp() const
{
    return d_timeStamp;
}

inline const bdlb::NullableValue<StatContextConfiguration>&
StatContextUpdate::configuration() const
{
    return d_configuration;
}

inline const bsl::vector<StatValueUpdate>&
StatContextUpdate::directValues() const
{
    return d_directValues;
}

inline const bsl::vector<StatValueUpdate>&
StatContextUpdate::expiredValues() const
{
    return d_expiredValues;
}

inline const bsl::vector<StatContextUpdate>&
StatContextUpdate::subcontexts() const
{
    return d_subcontexts;
}

// ---------------------------
// class StatContextUpdateList
// ---------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StatContextUpdateList::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_contexts,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONTEXTS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StatContextUpdateList::manipulateAttribute(t_MANIPULATOR& manipulator,
                                               int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CONTEXTS: {
        return manipulator(&d_contexts,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONTEXTS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StatContextUpdateList::manipulateAttribute(t_MANIPULATOR& manipulator,
                                               const char*    name,
                                               int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::vector<StatContextUpdate>& StatContextUpdateList::contexts()
{
    return d_contexts;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StatContextUpdateList::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_contexts, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONTEXTS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StatContextUpdateList::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CONTEXTS: {
        return accessor(d_contexts,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONTEXTS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StatContextUpdateList::accessAttribute(t_ACCESSOR& accessor,
                                           const char* name,
                                           int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::vector<StatContextUpdate>&
StatContextUpdateList::contexts() const
{
    return d_contexts;
}

}  // close package namespace

// FREE FUNCTIONS

}  // close enterprise namespace
#endif

// GENERATED BY BLP_BAS_CODEGEN_2024.05.02
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mwcstm --msgComponent values mwcstm.xsd
