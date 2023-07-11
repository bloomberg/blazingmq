// Copyright 2023 Bloomberg Finance L.P.
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

#include <bsls_ident.h>
BSLS_IDENT_RCSID(mwcstm_values_h, "$Id$ $CSID$")
BSLS_IDENT_PRAGMA_ONCE

//@PURPOSE: Provide value-semantic attribute classes

#include <bslalg_typetraits.h>

#include <bdlat_attributeinfo.h>

#include <bdlat_enumeratorinfo.h>

#include <bdlat_selectioninfo.h>

#include <bdlat_typetraits.h>

#include <bslh_hash.h>
#include <bsls_objectbuffer.h>

#include <bslx_instreamfunctions.h>
#include <bslx_outstreamfunctions.h>

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

    /// Return the most current `bdex` streaming version number supported by
    /// this class.  See the `bslx` package-level documentation for more
    /// information on `bdex` streaming of value-semantic types and
    /// containers.
    static int maxSupportedBdexVersion();

    /// Return selection information for the selection indicated by the
    /// specified `id` if the selection exists, and 0 otherwise.
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);

    /// Return selection information for the selection indicated by the
    /// specified `name` of the specified `nameLength` if the selection
    /// exists, and 0 otherwise.
    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `StatContextConfigurationChoice` having the
    /// default value.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    explicit StatContextConfigurationChoice(
        bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `StatContextConfigurationChoice` having the
    /// value of the specified `original` object.  Use the optionally
    /// specified `basicAllocator` to supply memory.  If `basicAllocator` is
    /// 0, the currently installed default allocator is used.
    StatContextConfigurationChoice(
        const StatContextConfigurationChoice& original,
        bslma::Allocator*                     basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StatContextConfigurationChoice` having the
    /// value of the specified `original` object.  After performing this
    /// action, the `original` object will be left in a valid, but
    /// unspecified state.
    StatContextConfigurationChoice(
        StatContextConfigurationChoice&& original) noexcept;

    /// Create an object of type `StatContextConfigurationChoice` having the
    /// value of the specified `original` object.  After performing this
    /// action, the `original` object will be left in a valid, but
    /// unspecified state.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    StatContextConfigurationChoice(StatContextConfigurationChoice&& original,
                                   bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~StatContextConfigurationChoice();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StatContextConfigurationChoice&
    operator=(const StatContextConfigurationChoice& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    StatContextConfigurationChoice&
    operator=(StatContextConfigurationChoice&& rhs);
#endif

    /// Assign to this object the value read from the specified input
    /// `stream` using the specified `version` format and return a reference
    /// to the modifiable `stream`.  If `stream` is initially invalid, this
    /// operation has no effect.  If `stream` becomes invalid during this
    /// operation, this object is valid, but its value is undefined.  If
    /// `version` is not supported, `stream` is marked invalid and this
    /// object is unaltered.  Note that no version is read from `stream`.
    /// See the `bslx` package-level documentation for more information on
    /// `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamIn(t_STREAM& stream, int version);

    /// Reset this object to the default value (i.e., its value upon default
    /// construction).
    void reset();

    /// Set the value of this object to be the default for the selection
    /// indicated by the specified `selectionId`.  Return 0 on success, and
    /// non-zero value otherwise (i.e., the selection is not found).
    int makeSelection(int selectionId);

    /// Set the value of this object to be the default for the selection
    /// indicated by the specified `name` of the specified `nameLength`.
    /// Return 0 on success, and non-zero value otherwise (i.e., the
    /// selection is not found).
    int makeSelection(const char* name, int nameLength);

    /// Set the value of this object to be a "Id" value.  Optionally specify
    /// the `value` of the "Id".  If `value` is not specified, the default
    /// "Id" value is used.
    bsls::Types::Int64& makeId();
    bsls::Types::Int64& makeId(bsls::Types::Int64 value);

    bsl::string& makeName();
    bsl::string& makeName(const bsl::string& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    bsl::string& makeName(bsl::string&& value);
#endif
    // Set the value of this object to be a "Name" value.  Optionally
    // specify the 'value' of the "Name".  If 'value' is not specified, the
    // default "Name" value is used.

    /// Invoke the specified `manipulator` on the address of the modifiable
    /// selection, supplying `manipulator` with the corresponding selection
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if this object has a defined selection,
    /// and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);

    /// Return a reference to the modifiable "Id" selection of this object
    /// if "Id" is the current selection.  The behavior is undefined unless
    /// "Id" is the selection of this object.
    bsls::Types::Int64& id();

    /// Return a reference to the modifiable "Name" selection of this object
    /// if "Name" is the current selection.  The behavior is undefined
    /// unless "Name" is the selection of this object.
    bsl::string& name();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Write the value of this object to the specified output `stream`
    /// using the specified `version` format and return a reference to the
    /// modifiable `stream`.  If `version` is not supported, `stream` is
    /// unmodified.  Note that `version` is not written to `stream`.
    /// See the `bslx` package-level documentation for more information
    /// on `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamOut(t_STREAM& stream, int version) const;

    /// Return the id of the current selection if the selection is defined,
    /// and -1 otherwise.
    int selectionId() const;

    /// Invoke the specified `accessor` on the non-modifiable selection,
    /// supplying `accessor` with the corresponding selection information
    /// structure.  Return the value returned from the invocation of
    /// `accessor` if this object has a defined selection, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;

    /// Return a reference to the non-modifiable "Id" selection of this
    /// object if "Id" is the current selection.  The behavior is undefined
    /// unless "Id" is the selection of this object.
    const bsls::Types::Int64& id() const;

    /// Return a reference to the non-modifiable "Name" selection of this
    /// object if "Name" is the current selection.  The behavior is
    /// undefined unless "Name" is the selection of this object.
    const bsl::string& name() const;

    /// Return `true` if the value of this object is a "Id" value, and
    /// return `false` otherwise.
    bool isIdValue() const;

    /// Return `true` if the value of this object is a "Name" value, and
    /// return `false` otherwise.
    bool isNameValue() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefinedValue() const;

    /// Return the symbolic name of the current selection of this object.
    const char* selectionName() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` objects have the same
/// value, and `false` otherwise.  Two `StatContextConfigurationChoice` objects
/// have the same value if either the selections in both objects have the same
/// ids and the same values, or both selections are undefined.
inline bool operator==(const StatContextConfigurationChoice& lhs,
                       const StatContextConfigurationChoice& rhs);

/// Return `true` if the specified `lhs` and `rhs` objects do not have the
/// same values, as determined by `operator==`, and `false` otherwise.
inline bool operator!=(const StatContextConfigurationChoice& lhs,
                       const StatContextConfigurationChoice& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&                         stream,
                                const StatContextConfigurationChoice& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `StatContextConfigurationChoice`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM&                     hashAlg,
                const StatContextConfigurationChoice& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatContextConfigurationChoice)

namespace mwcstm {

// ===================================
// class StatContextConfigurationFlags
// ===================================

/// This type enumerates the flags that can be set on a
/// `StatContextConfiguration`.  A configuration having the `IS_TABLE` flag
/// denotes that it is a table context, and `STORE_EXPIRED_VALUES` indicates
/// it should remember the values of expired subcontexts.
struct StatContextConfigurationFlags {
  public:
    // TYPES
    enum Value { DMCSTM_IS_TABLE = 0, DMCSTM_STORE_EXPIRED_VALUES = 1 };

    enum { NUM_ENUMERATORS = 2 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS

    /// Return the most current `bdex` streaming version number supported by
    /// this class.  See the `bslx` package-level documentation for more
    /// information on `bdex` streaming of value-semantic types and
    /// containers.
    static int maxSupportedBdexVersion();

    /// Return the string representation exactly matching the enumerator
    /// name corresponding to the specified enumeration `value`.
    static const char* toString(Value value);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string` of the specified `stringLength`.  Return 0 on
    /// success, and a non-zero value with no effect on `result` otherwise
    /// (i.e., `string` does not match any enumerator).
    static int fromString(Value* result, const char* string, int stringLength);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `string` does not match any
    /// enumerator).
    static int fromString(Value* result, const bsl::string& string);

    /// Load into the specified `result` the enumerator matching the
    /// specified `number`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `number` does not match any
    /// enumerator).
    static int fromInt(Value* result, int number);

    /// Assign to the specified `value` the value read from the specified
    /// input `stream` using the specified `version` format and return a
    /// reference to the modifiable `stream`.  If `stream` is initially
    /// invalid, this operation has no effect.  If `stream` becomes invalid
    /// during this operation, the `value` is valid, but its value is
    /// undefined.  If the specified `version` is not supported, `stream` is
    /// marked invalid, but `value` is unaltered.  Note that no version is
    /// read from `stream`.  (See the package-group-level documentation for
    /// more information on `bdex` streaming of container types.)
    template <typename t_STREAM>
    static t_STREAM& bdexStreamIn(t_STREAM& stream, Value& value, int version);

    /// Write to the specified `stream` the string representation of
    /// the specified enumeration `value`.  Return a reference to
    /// the modifiable `stream`.
    static bsl::ostream& print(bsl::ostream& stream, Value value);

    /// Write the specified `value` to the specified output `stream` and
    /// return a reference to the modifiable `stream`.  Optionally specify
    /// an explicit `version` format; by default, the maximum supported
    /// version is written to `stream` and used as the format.  If `version`
    /// is specified, that format is used but *not* written to `stream`.  If
    /// `version` is not supported, `stream` is left unmodified.  (See the
    /// package-group-level documentation for more information on `bdex`
    /// streaming of container types).
    template <typename t_STREAM>
    static t_STREAM& bdexStreamOut(t_STREAM& stream, Value value, int version);
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&                        stream,
                                StatContextConfigurationFlags::Value rhs);

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mwcstm::StatContextConfigurationFlags)

namespace mwcstm {

// ============================
// class StatContextUpdateFlags
// ============================

/// This type enumerates the flags that can be set on a `StatContext`.  A
/// context having the `CONTEXT_CREATED` flag indicates that the context was
/// just created, and the `CONTEXT_DELETED` flag indicates that it was
/// deleted.
struct StatContextUpdateFlags {
  public:
    // TYPES
    enum Value { DMCSTM_CONTEXT_CREATED = 0, DMCSTM_CONTEXT_DELETED = 1 };

    enum { NUM_ENUMERATORS = 2 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS

    /// Return the most current `bdex` streaming version number supported by
    /// this class.  See the `bslx` package-level documentation for more
    /// information on `bdex` streaming of value-semantic types and
    /// containers.
    static int maxSupportedBdexVersion();

    /// Return the string representation exactly matching the enumerator
    /// name corresponding to the specified enumeration `value`.
    static const char* toString(Value value);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string` of the specified `stringLength`.  Return 0 on
    /// success, and a non-zero value with no effect on `result` otherwise
    /// (i.e., `string` does not match any enumerator).
    static int fromString(Value* result, const char* string, int stringLength);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `string` does not match any
    /// enumerator).
    static int fromString(Value* result, const bsl::string& string);

    /// Load into the specified `result` the enumerator matching the
    /// specified `number`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `number` does not match any
    /// enumerator).
    static int fromInt(Value* result, int number);

    /// Assign to the specified `value` the value read from the specified
    /// input `stream` using the specified `version` format and return a
    /// reference to the modifiable `stream`.  If `stream` is initially
    /// invalid, this operation has no effect.  If `stream` becomes invalid
    /// during this operation, the `value` is valid, but its value is
    /// undefined.  If the specified `version` is not supported, `stream` is
    /// marked invalid, but `value` is unaltered.  Note that no version is
    /// read from `stream`.  (See the package-group-level documentation for
    /// more information on `bdex` streaming of container types.)
    template <typename t_STREAM>
    static t_STREAM& bdexStreamIn(t_STREAM& stream, Value& value, int version);

    /// Write to the specified `stream` the string representation of
    /// the specified enumeration `value`.  Return a reference to
    /// the modifiable `stream`.
    static bsl::ostream& print(bsl::ostream& stream, Value value);

    /// Write the specified `value` to the specified output `stream` and
    /// return a reference to the modifiable `stream`.  Optionally specify
    /// an explicit `version` format; by default, the maximum supported
    /// version is written to `stream` and used as the format.  If `version`
    /// is specified, that format is used but *not* written to `stream`.  If
    /// `version` is not supported, `stream` is left unmodified.  (See the
    /// package-group-level documentation for more information on `bdex`
    /// streaming of container types).
    template <typename t_STREAM>
    static t_STREAM& bdexStreamOut(t_STREAM& stream, Value value, int version);
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&                 stream,
                                StatContextUpdateFlags::Value rhs);

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mwcstm::StatContextUpdateFlags)

namespace mwcstm {

// =====================
// class StatValueFields
// =====================

/// This type enumerates the different bit indices corresponding to fields
/// in a stat value.
struct StatValueFields {
  public:
    // TYPES
    enum Value {
        DMCSTM_ABSOLUTE_MIN = 0,
        DMCSTM_ABSOLUTE_MAX = 1,
        DMCSTM_MIN          = 2,
        DMCSTM_MAX          = 3,
        DMCSTM_EVENTS       = 4,
        DMCSTM_SUM          = 5,
        DMCSTM_VALUE        = 6,
        DMCSTM_INCREMENTS   = 7,
        DMCSTM_DECREMENTS   = 8
    };

    enum { NUM_ENUMERATORS = 9 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS

    /// Return the most current `bdex` streaming version number supported by
    /// this class.  See the `bslx` package-level documentation for more
    /// information on `bdex` streaming of value-semantic types and
    /// containers.
    static int maxSupportedBdexVersion();

    /// Return the string representation exactly matching the enumerator
    /// name corresponding to the specified enumeration `value`.
    static const char* toString(Value value);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string` of the specified `stringLength`.  Return 0 on
    /// success, and a non-zero value with no effect on `result` otherwise
    /// (i.e., `string` does not match any enumerator).
    static int fromString(Value* result, const char* string, int stringLength);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `string` does not match any
    /// enumerator).
    static int fromString(Value* result, const bsl::string& string);

    /// Load into the specified `result` the enumerator matching the
    /// specified `number`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `number` does not match any
    /// enumerator).
    static int fromInt(Value* result, int number);

    /// Assign to the specified `value` the value read from the specified
    /// input `stream` using the specified `version` format and return a
    /// reference to the modifiable `stream`.  If `stream` is initially
    /// invalid, this operation has no effect.  If `stream` becomes invalid
    /// during this operation, the `value` is valid, but its value is
    /// undefined.  If the specified `version` is not supported, `stream` is
    /// marked invalid, but `value` is unaltered.  Note that no version is
    /// read from `stream`.  (See the package-group-level documentation for
    /// more information on `bdex` streaming of container types.)
    template <typename t_STREAM>
    static t_STREAM& bdexStreamIn(t_STREAM& stream, Value& value, int version);

    /// Write to the specified `stream` the string representation of
    /// the specified enumeration `value`.  Return a reference to
    /// the modifiable `stream`.
    static bsl::ostream& print(bsl::ostream& stream, Value value);

    /// Write the specified `value` to the specified output `stream` and
    /// return a reference to the modifiable `stream`.  Optionally specify
    /// an explicit `version` format; by default, the maximum supported
    /// version is written to `stream` and used as the format.  If `version`
    /// is specified, that format is used but *not* written to `stream`.  If
    /// `version` is not supported, `stream` is left unmodified.  (See the
    /// package-group-level documentation for more information on `bdex`
    /// streaming of container types).
    template <typename t_STREAM>
    static t_STREAM& bdexStreamOut(t_STREAM& stream, Value value, int version);
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&          stream,
                                StatValueFields::Value rhs);

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mwcstm::StatValueFields)

namespace mwcstm {

// ===================
// class StatValueType
// ===================

/// This type enumerates the different types of stat values, specifically
/// `CONTINUOUS` and `DISCRETE` values.
struct StatValueType {
  public:
    // TYPES
    enum Value { DMCSTM_CONTINUOUS = 0, DMCSTM_DISCRETE = 1 };

    enum { NUM_ENUMERATORS = 2 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS

    /// Return the most current `bdex` streaming version number supported by
    /// this class.  See the `bslx` package-level documentation for more
    /// information on `bdex` streaming of value-semantic types and
    /// containers.
    static int maxSupportedBdexVersion();

    /// Return the string representation exactly matching the enumerator
    /// name corresponding to the specified enumeration `value`.
    static const char* toString(Value value);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string` of the specified `stringLength`.  Return 0 on
    /// success, and a non-zero value with no effect on `result` otherwise
    /// (i.e., `string` does not match any enumerator).
    static int fromString(Value* result, const char* string, int stringLength);

    /// Load into the specified `result` the enumerator matching the
    /// specified `string`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `string` does not match any
    /// enumerator).
    static int fromString(Value* result, const bsl::string& string);

    /// Load into the specified `result` the enumerator matching the
    /// specified `number`.  Return 0 on success, and a non-zero value with
    /// no effect on `result` otherwise (i.e., `number` does not match any
    /// enumerator).
    static int fromInt(Value* result, int number);

    /// Assign to the specified `value` the value read from the specified
    /// input `stream` using the specified `version` format and return a
    /// reference to the modifiable `stream`.  If `stream` is initially
    /// invalid, this operation has no effect.  If `stream` becomes invalid
    /// during this operation, the `value` is valid, but its value is
    /// undefined.  If the specified `version` is not supported, `stream` is
    /// marked invalid, but `value` is unaltered.  Note that no version is
    /// read from `stream`.  (See the package-group-level documentation for
    /// more information on `bdex` streaming of container types.)
    template <typename t_STREAM>
    static t_STREAM& bdexStreamIn(t_STREAM& stream, Value& value, int version);

    /// Write to the specified `stream` the string representation of
    /// the specified enumeration `value`.  Return a reference to
    /// the modifiable `stream`.
    static bsl::ostream& print(bsl::ostream& stream, Value value);

    /// Write the specified `value` to the specified output `stream` and
    /// return a reference to the modifiable `stream`.  Optionally specify
    /// an explicit `version` format; by default, the maximum supported
    /// version is written to `stream` and used as the format.  If `version`
    /// is specified, that format is used but *not* written to `stream`.  If
    /// `version` is not supported, `stream` is left unmodified.  (See the
    /// package-group-level documentation for more information on `bdex`
    /// streaming of container types).
    template <typename t_STREAM>
    static t_STREAM& bdexStreamOut(t_STREAM& stream, Value value, int version);
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&        stream,
                                StatValueType::Value rhs);

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(mwcstm::StatValueType)

namespace mwcstm {

// =====================
// class StatValueUpdate
// =====================

/// This type represents changes made to a particular state value.  The bits
/// of the `fieldMask` indicate which actual fields in a stat value the
/// elements of `fields` are updating.  For instance, if the value in
/// question is a continuous value, and `fieldMask` is 0b001100, then
/// `fields` should contain two elements, with `fields[0]` being the update
/// to the `min` value, and `fields[1]` being the update to the `max` value.
class StatValueUpdate {
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

    /// Return the most current `bdex` streaming version number supported by
    /// this class.  See the `bslx` package-level documentation for more
    /// information on `bdex` streaming of value-semantic types and
    /// containers.
    static int maxSupportedBdexVersion();

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `StatValueUpdate` having the default value.
    ///  Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    explicit StatValueUpdate(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `StatValueUpdate` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    StatValueUpdate(const StatValueUpdate& original,
                    bslma::Allocator*      basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StatValueUpdate` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    StatValueUpdate(StatValueUpdate&& original) noexcept;

    /// Create an object of type `StatValueUpdate` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    StatValueUpdate(StatValueUpdate&& original,
                    bslma::Allocator* basicAllocator);
#endif

    /// Destroy this object.
    ~StatValueUpdate();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StatValueUpdate& operator=(const StatValueUpdate& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    StatValueUpdate& operator=(StatValueUpdate&& rhs);
#endif

    /// Assign to this object the value read from the specified input
    /// `stream` using the specified `version` format and return a reference
    /// to the modifiable `stream`.  If `stream` is initially invalid, this
    /// operation has no effect.  If `stream` becomes invalid during this
    /// operation, this object is valid, but its value is undefined.  If
    /// `version` is not supported, `stream` is marked invalid and this
    /// object is unaltered.  Note that no version is read from `stream`.
    /// See the `bslx` package-level documentation for more information on
    /// `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamIn(t_STREAM& stream, int version);

    /// Reset this object to the default value (i.e., its value upon
    /// default construction).
    void reset();

    /// Invoke the specified `manipulator` sequentially on the address of
    /// each (modifiable) attribute of this object, supplying `manipulator`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `manipulator` (i.e., the invocation that
    /// terminated the sequence).
    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `id`,
    /// supplying `manipulator` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if `id` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `name` of the
    /// specified `nameLength`, supplying `manipulator` with the
    /// corresponding attribute information structure.  Return the value
    /// returned from the invocation of `manipulator` if `name` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);

    /// Return a reference to the modifiable "FieldMask" attribute of this
    /// object.
    unsigned int& fieldMask();

    /// Return a reference to the modifiable "Fields" attribute of this
    /// object.
    bsl::vector<bsls::Types::Int64>& fields();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Write the value of this object to the specified output `stream`
    /// using the specified `version` format and return a reference to the
    /// modifiable `stream`.  If `version` is not supported, `stream` is
    /// unmodified.  Note that `version` is not written to `stream`.
    /// See the `bslx` package-level documentation for more information
    /// on `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamOut(t_STREAM& stream, int version) const;

    /// Invoke the specified `accessor` sequentially on each
    /// (non-modifiable) attribute of this object, supplying `accessor`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `accessor` (i.e., the invocation that terminated
    /// the sequence).
    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `id`, supplying `accessor`
    /// with the corresponding attribute information structure.  Return the
    /// value returned from the invocation of `accessor` if `id` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `name` of the specified
    /// `nameLength`, supplying `accessor` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `accessor` if `name` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;

    /// Return the value of the "FieldMask" attribute of this object.
    unsigned int fieldMask() const;

    /// Return a reference offering non-modifiable access to the "Fields"
    /// attribute of this object.
    const bsl::vector<bsls::Types::Int64>& fields() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const StatValueUpdate& lhs, const StatValueUpdate& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const StatValueUpdate& lhs, const StatValueUpdate& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&          stream,
                                const StatValueUpdate& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `StatValueUpdate`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const StatValueUpdate& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatValueUpdate)

namespace mwcstm {

// =========================
// class StatValueDefinition
// =========================

/// This type represents the definition of a stat collecting value, having a
/// `name`, a `type` and an array of `historySizes` indicating the number of
/// snapshots and levels of snapshots to keep.
class StatValueDefinition {
    // INSTANCE DATA
    bsl::vector<unsigned int> d_historySizes;
    bsl::string               d_name;
    StatValueType::Value      d_type;

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

    /// Return the most current `bdex` streaming version number supported by
    /// this class.  See the `bslx` package-level documentation for more
    /// information on `bdex` streaming of value-semantic types and
    /// containers.
    static int maxSupportedBdexVersion();

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `StatValueDefinition` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply
    /// memory.  If `basicAllocator` is 0, the currently installed default
    /// allocator is used.
    explicit StatValueDefinition(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `StatValueDefinition` having the value of
    /// the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    StatValueDefinition(const StatValueDefinition& original,
                        bslma::Allocator*          basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StatValueDefinition` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    StatValueDefinition(StatValueDefinition&& original) noexcept;

    /// Create an object of type `StatValueDefinition` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    StatValueDefinition(StatValueDefinition&& original,
                        bslma::Allocator*     basicAllocator);
#endif

    /// Destroy this object.
    ~StatValueDefinition();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StatValueDefinition& operator=(const StatValueDefinition& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    StatValueDefinition& operator=(StatValueDefinition&& rhs);
#endif

    /// Assign to this object the value read from the specified input
    /// `stream` using the specified `version` format and return a reference
    /// to the modifiable `stream`.  If `stream` is initially invalid, this
    /// operation has no effect.  If `stream` becomes invalid during this
    /// operation, this object is valid, but its value is undefined.  If
    /// `version` is not supported, `stream` is marked invalid and this
    /// object is unaltered.  Note that no version is read from `stream`.
    /// See the `bslx` package-level documentation for more information on
    /// `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamIn(t_STREAM& stream, int version);

    /// Reset this object to the default value (i.e., its value upon
    /// default construction).
    void reset();

    /// Invoke the specified `manipulator` sequentially on the address of
    /// each (modifiable) attribute of this object, supplying `manipulator`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `manipulator` (i.e., the invocation that
    /// terminated the sequence).
    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `id`,
    /// supplying `manipulator` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if `id` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `name` of the
    /// specified `nameLength`, supplying `manipulator` with the
    /// corresponding attribute information structure.  Return the value
    /// returned from the invocation of `manipulator` if `name` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);

    /// Return a reference to the modifiable "Name" attribute of this
    /// object.
    bsl::string& name();

    /// Return a reference to the modifiable "Type" attribute of this
    /// object.
    StatValueType::Value& type();

    /// Return a reference to the modifiable "HistorySizes" attribute of
    /// this object.
    bsl::vector<unsigned int>& historySizes();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Write the value of this object to the specified output `stream`
    /// using the specified `version` format and return a reference to the
    /// modifiable `stream`.  If `version` is not supported, `stream` is
    /// unmodified.  Note that `version` is not written to `stream`.
    /// See the `bslx` package-level documentation for more information
    /// on `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamOut(t_STREAM& stream, int version) const;

    /// Invoke the specified `accessor` sequentially on each
    /// (non-modifiable) attribute of this object, supplying `accessor`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `accessor` (i.e., the invocation that terminated
    /// the sequence).
    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `id`, supplying `accessor`
    /// with the corresponding attribute information structure.  Return the
    /// value returned from the invocation of `accessor` if `id` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `name` of the specified
    /// `nameLength`, supplying `accessor` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `accessor` if `name` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;

    /// Return a reference offering non-modifiable access to the "Name"
    /// attribute of this object.
    const bsl::string& name() const;

    /// Return the value of the "Type" attribute of this object.
    StatValueType::Value type() const;

    /// Return a reference offering non-modifiable access to the
    /// "HistorySizes" attribute of this object.
    const bsl::vector<unsigned int>& historySizes() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const StatValueDefinition& lhs,
                       const StatValueDefinition& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const StatValueDefinition& lhs,
                       const StatValueDefinition& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&              stream,
                                const StatValueDefinition& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `StatValueDefinition`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const StatValueDefinition& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatValueDefinition)

namespace mwcstm {

// ==============================
// class StatContextConfiguration
// ==============================

/// This type represents the configuration of a stat context, having some
/// configuration `flags`, a user supplied `userId` that may either be a
/// long integer `id` or a string `name`, and a set of `values` to track.
/// Note that `values` may be omitted if this configuration is for a
/// subcontext of a table context.
class StatContextConfiguration {
    // INSTANCE DATA
    bsl::vector<StatValueDefinition> d_values;
    StatContextConfigurationChoice   d_choice;
    unsigned int                     d_flags;

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

    /// Return the most current `bdex` streaming version number supported by
    /// this class.  See the `bslx` package-level documentation for more
    /// information on `bdex` streaming of value-semantic types and
    /// containers.
    static int maxSupportedBdexVersion();

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `StatContextConfiguration` having the
    /// default value.  Use the optionally specified `basicAllocator` to
    /// supply memory.  If `basicAllocator` is 0, the currently installed
    /// default allocator is used.
    explicit StatContextConfiguration(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `StatContextConfiguration` having the value
    /// of the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    StatContextConfiguration(const StatContextConfiguration& original,
                             bslma::Allocator* basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StatContextConfiguration` having the value
    /// of the specified `original` object.  After performing this action,
    /// the `original` object will be left in a valid, but unspecified
    /// state.
    StatContextConfiguration(StatContextConfiguration&& original) noexcept;

    /// Create an object of type `StatContextConfiguration` having the value
    /// of the specified `original` object.  After performing this action,
    /// the `original` object will be left in a valid, but unspecified
    /// state.  Use the optionally specified `basicAllocator` to supply
    /// memory.  If `basicAllocator` is 0, the currently installed default
    /// allocator is used.
    StatContextConfiguration(StatContextConfiguration&& original,
                             bslma::Allocator*          basicAllocator);
#endif

    /// Destroy this object.
    ~StatContextConfiguration();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StatContextConfiguration& operator=(const StatContextConfiguration& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    StatContextConfiguration& operator=(StatContextConfiguration&& rhs);
#endif

    /// Assign to this object the value read from the specified input
    /// `stream` using the specified `version` format and return a reference
    /// to the modifiable `stream`.  If `stream` is initially invalid, this
    /// operation has no effect.  If `stream` becomes invalid during this
    /// operation, this object is valid, but its value is undefined.  If
    /// `version` is not supported, `stream` is marked invalid and this
    /// object is unaltered.  Note that no version is read from `stream`.
    /// See the `bslx` package-level documentation for more information on
    /// `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamIn(t_STREAM& stream, int version);

    /// Reset this object to the default value (i.e., its value upon
    /// default construction).
    void reset();

    /// Invoke the specified `manipulator` sequentially on the address of
    /// each (modifiable) attribute of this object, supplying `manipulator`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `manipulator` (i.e., the invocation that
    /// terminated the sequence).
    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `id`,
    /// supplying `manipulator` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if `id` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `name` of the
    /// specified `nameLength`, supplying `manipulator` with the
    /// corresponding attribute information structure.  Return the value
    /// returned from the invocation of `manipulator` if `name` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);

    /// Return a reference to the modifiable "Flags" attribute of this
    /// object.
    unsigned int& flags();

    /// Return a reference to the modifiable "Choice" attribute of this
    /// object.
    StatContextConfigurationChoice& choice();

    /// Return a reference to the modifiable "Values" attribute of this
    /// object.
    bsl::vector<StatValueDefinition>& values();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Write the value of this object to the specified output `stream`
    /// using the specified `version` format and return a reference to the
    /// modifiable `stream`.  If `version` is not supported, `stream` is
    /// unmodified.  Note that `version` is not written to `stream`.
    /// See the `bslx` package-level documentation for more information
    /// on `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamOut(t_STREAM& stream, int version) const;

    /// Invoke the specified `accessor` sequentially on each
    /// (non-modifiable) attribute of this object, supplying `accessor`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `accessor` (i.e., the invocation that terminated
    /// the sequence).
    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `id`, supplying `accessor`
    /// with the corresponding attribute information structure.  Return the
    /// value returned from the invocation of `accessor` if `id` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `name` of the specified
    /// `nameLength`, supplying `accessor` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `accessor` if `name` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;

    /// Return the value of the "Flags" attribute of this object.
    unsigned int flags() const;

    /// Return a reference offering non-modifiable access to the "Choice"
    /// attribute of this object.
    const StatContextConfigurationChoice& choice() const;

    /// Return a reference offering non-modifiable access to the "Values"
    /// attribute of this object.
    const bsl::vector<StatValueDefinition>& values() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const StatContextConfiguration& lhs,
                       const StatContextConfiguration& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const StatContextConfiguration& lhs,
                       const StatContextConfiguration& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&                   stream,
                                const StatContextConfiguration& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `StatContextConfiguration`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM&               hashAlg,
                const StatContextConfiguration& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatContextConfiguration)

namespace mwcstm {

// =======================
// class StatContextUpdate
// =======================

/// This type represents an externalizable `StatContext`, having an integer
/// `id` distinct from all sibling contexts, a set of `flags` denoting if
/// this is a new or deleted context, an optional `configuration` that
/// describes the context structure that is excluded if redundant, a set of
/// `directValues` and `expiredValues`, and a set of `subcontexts`.
class StatContextUpdate {
    // INSTANCE DATA
    bsls::Types::Int64                            d_timeStamp;
    bsl::vector<StatValueUpdate>                  d_directValues;
    bsl::vector<StatValueUpdate>                  d_expiredValues;
    bsl::vector<StatContextUpdate>                d_subcontexts;
    bdlb::NullableValue<StatContextConfiguration> d_configuration;
    unsigned int                                  d_flags;
    int                                           d_id;

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

    /// Return the most current `bdex` streaming version number supported by
    /// this class.  See the `bslx` package-level documentation for more
    /// information on `bdex` streaming of value-semantic types and
    /// containers.
    static int maxSupportedBdexVersion();

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `StatContextUpdate` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply
    /// memory.  If `basicAllocator` is 0, the currently installed default
    /// allocator is used.
    explicit StatContextUpdate(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `StatContextUpdate` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    StatContextUpdate(const StatContextUpdate& original,
                      bslma::Allocator*        basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StatContextUpdate` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    StatContextUpdate(StatContextUpdate&& original) noexcept;

    /// Create an object of type `StatContextUpdate` having the value of the
    /// specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    StatContextUpdate(StatContextUpdate&& original,
                      bslma::Allocator*   basicAllocator);
#endif

    /// Destroy this object.
    ~StatContextUpdate();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StatContextUpdate& operator=(const StatContextUpdate& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    StatContextUpdate& operator=(StatContextUpdate&& rhs);
#endif

    /// Assign to this object the value read from the specified input
    /// `stream` using the specified `version` format and return a reference
    /// to the modifiable `stream`.  If `stream` is initially invalid, this
    /// operation has no effect.  If `stream` becomes invalid during this
    /// operation, this object is valid, but its value is undefined.  If
    /// `version` is not supported, `stream` is marked invalid and this
    /// object is unaltered.  Note that no version is read from `stream`.
    /// See the `bslx` package-level documentation for more information on
    /// `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamIn(t_STREAM& stream, int version);

    /// Reset this object to the default value (i.e., its value upon
    /// default construction).
    void reset();

    /// Invoke the specified `manipulator` sequentially on the address of
    /// each (modifiable) attribute of this object, supplying `manipulator`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `manipulator` (i.e., the invocation that
    /// terminated the sequence).
    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `id`,
    /// supplying `manipulator` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if `id` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `name` of the
    /// specified `nameLength`, supplying `manipulator` with the
    /// corresponding attribute information structure.  Return the value
    /// returned from the invocation of `manipulator` if `name` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);

    /// Return a reference to the modifiable "Id" attribute of this object.
    int& id();

    /// Return a reference to the modifiable "Flags" attribute of this
    /// object.
    unsigned int& flags();

    /// Return a reference to the modifiable "TimeStamp" attribute of this
    /// object.
    bsls::Types::Int64& timeStamp();

    /// Return a reference to the modifiable "Configuration" attribute of
    /// this object.
    bdlb::NullableValue<StatContextConfiguration>& configuration();

    /// Return a reference to the modifiable "DirectValues" attribute of
    /// this object.
    bsl::vector<StatValueUpdate>& directValues();

    /// Return a reference to the modifiable "ExpiredValues" attribute of
    /// this object.
    bsl::vector<StatValueUpdate>& expiredValues();

    /// Return a reference to the modifiable "Subcontexts" attribute of this
    /// object.
    bsl::vector<StatContextUpdate>& subcontexts();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Write the value of this object to the specified output `stream`
    /// using the specified `version` format and return a reference to the
    /// modifiable `stream`.  If `version` is not supported, `stream` is
    /// unmodified.  Note that `version` is not written to `stream`.
    /// See the `bslx` package-level documentation for more information
    /// on `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamOut(t_STREAM& stream, int version) const;

    /// Invoke the specified `accessor` sequentially on each
    /// (non-modifiable) attribute of this object, supplying `accessor`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `accessor` (i.e., the invocation that terminated
    /// the sequence).
    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `id`, supplying `accessor`
    /// with the corresponding attribute information structure.  Return the
    /// value returned from the invocation of `accessor` if `id` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `name` of the specified
    /// `nameLength`, supplying `accessor` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `accessor` if `name` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;

    /// Return the value of the "Id" attribute of this object.
    int id() const;

    /// Return the value of the "Flags" attribute of this object.
    unsigned int flags() const;

    /// Return the value of the "TimeStamp" attribute of this object.
    bsls::Types::Int64 timeStamp() const;

    /// Return a reference offering non-modifiable access to the
    /// "Configuration" attribute of this object.
    const bdlb::NullableValue<StatContextConfiguration>& configuration() const;

    /// Return a reference offering non-modifiable access to the
    /// "DirectValues" attribute of this object.
    const bsl::vector<StatValueUpdate>& directValues() const;

    /// Return a reference offering non-modifiable access to the
    /// "ExpiredValues" attribute of this object.
    const bsl::vector<StatValueUpdate>& expiredValues() const;

    /// Return a reference offering non-modifiable access to the
    /// "Subcontexts" attribute of this object.
    const bsl::vector<StatContextUpdate>& subcontexts() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const StatContextUpdate& lhs,
                       const StatContextUpdate& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const StatContextUpdate& lhs,
                       const StatContextUpdate& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&            stream,
                                const StatContextUpdate& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `StatContextUpdate`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const StatContextUpdate& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatContextUpdate)

namespace mwcstm {

// ===========================
// class StatContextUpdateList
// ===========================

/// This type represents a sequence of `StatContext` objects.
class StatContextUpdateList {
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

    /// Return the most current `bdex` streaming version number supported by
    /// this class.  See the `bslx` package-level documentation for more
    /// information on `bdex` streaming of value-semantic types and
    /// containers.
    static int maxSupportedBdexVersion();

    /// Return attribute information for the attribute indicated by the
    /// specified `id` if the attribute exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);

    /// Return attribute information for the attribute indicated by the
    /// specified `name` of the specified `nameLength` if the attribute
    /// exists, and 0 otherwise.
    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);

    // CREATORS

    /// Create an object of type `StatContextUpdateList` having the default
    /// value.  Use the optionally specified `basicAllocator` to supply
    /// memory.  If `basicAllocator` is 0, the currently installed default
    /// allocator is used.
    explicit StatContextUpdateList(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `StatContextUpdateList` having the value of
    /// the specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    StatContextUpdateList(const StatContextUpdateList& original,
                          bslma::Allocator*            basicAllocator = 0);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Create an object of type `StatContextUpdateList` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    StatContextUpdateList(StatContextUpdateList&& original) noexcept;

    /// Create an object of type `StatContextUpdateList` having the value of
    /// the specified `original` object.  After performing this action, the
    /// `original` object will be left in a valid, but unspecified state.
    /// Use the optionally specified `basicAllocator` to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    StatContextUpdateList(StatContextUpdateList&& original,
                          bslma::Allocator*       basicAllocator);
#endif

    /// Destroy this object.
    ~StatContextUpdateList();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    StatContextUpdateList& operator=(const StatContextUpdateList& rhs);

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    /// Assign to this object the value of the specified `rhs` object.
    /// After performing this action, the `rhs` object will be left in a
    /// valid, but unspecified state.
    StatContextUpdateList& operator=(StatContextUpdateList&& rhs);
#endif

    /// Assign to this object the value read from the specified input
    /// `stream` using the specified `version` format and return a reference
    /// to the modifiable `stream`.  If `stream` is initially invalid, this
    /// operation has no effect.  If `stream` becomes invalid during this
    /// operation, this object is valid, but its value is undefined.  If
    /// `version` is not supported, `stream` is marked invalid and this
    /// object is unaltered.  Note that no version is read from `stream`.
    /// See the `bslx` package-level documentation for more information on
    /// `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamIn(t_STREAM& stream, int version);

    /// Reset this object to the default value (i.e., its value upon
    /// default construction).
    void reset();

    /// Invoke the specified `manipulator` sequentially on the address of
    /// each (modifiable) attribute of this object, supplying `manipulator`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `manipulator` (i.e., the invocation that
    /// terminated the sequence).
    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `id`,
    /// supplying `manipulator` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `manipulator` if `id` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);

    /// Invoke the specified `manipulator` on the address of
    /// the (modifiable) attribute indicated by the specified `name` of the
    /// specified `nameLength`, supplying `manipulator` with the
    /// corresponding attribute information structure.  Return the value
    /// returned from the invocation of `manipulator` if `name` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);

    /// Return a reference to the modifiable "Contexts" attribute of this
    /// object.
    bsl::vector<StatContextUpdate>& contexts();

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Write the value of this object to the specified output `stream`
    /// using the specified `version` format and return a reference to the
    /// modifiable `stream`.  If `version` is not supported, `stream` is
    /// unmodified.  Note that `version` is not written to `stream`.
    /// See the `bslx` package-level documentation for more information
    /// on `bdex` streaming of value-semantic types and containers.
    template <typename t_STREAM>
    t_STREAM& bdexStreamOut(t_STREAM& stream, int version) const;

    /// Invoke the specified `accessor` sequentially on each
    /// (non-modifiable) attribute of this object, supplying `accessor`
    /// with the corresponding attribute information structure until such
    /// invocation returns a non-zero value.  Return the value from the
    /// last invocation of `accessor` (i.e., the invocation that terminated
    /// the sequence).
    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `id`, supplying `accessor`
    /// with the corresponding attribute information structure.  Return the
    /// value returned from the invocation of `accessor` if `id` identifies
    /// an attribute of this class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;

    /// Invoke the specified `accessor` on the (non-modifiable) attribute
    /// of this object indicated by the specified `name` of the specified
    /// `nameLength`, supplying `accessor` with the corresponding attribute
    /// information structure.  Return the value returned from the
    /// invocation of `accessor` if `name` identifies an attribute of this
    /// class, and -1 otherwise.
    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;

    /// Return a reference offering non-modifiable access to the "Contexts"
    /// attribute of this object.
    const bsl::vector<StatContextUpdate>& contexts() const;
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` and `rhs` attribute objects have
/// the same value, and `false` otherwise.  Two attribute objects have the
/// same value if each respective attribute has the same value.
inline bool operator==(const StatContextUpdateList& lhs,
                       const StatContextUpdateList& rhs);

/// Return `true` if the specified `lhs` and `rhs` attribute objects do not
/// have the same value, and `false` otherwise.  Two attribute objects do
/// not have the same value if one or more respective attributes differ in
/// values.
inline bool operator!=(const StatContextUpdateList& lhs,
                       const StatContextUpdateList& rhs);

/// Format the specified `rhs` to the specified output `stream` and
/// return a reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&                stream,
                                const StatContextUpdateList& rhs);

/// Pass the specified `object` to the specified `hashAlg`.  This function
/// integrates with the `bslh` modular hashing system and effectively
/// provides a `bsl::hash` specialization for `StatContextUpdateList`.
template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                const StatContextUpdateList& object);

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    mwcstm::StatContextUpdateList)

// ============================================================================
//                         INLINE FUNCTION DEFINITIONS
// ============================================================================

namespace mwcstm {

// ------------------------------------
// class StatContextConfigurationChoice
// ------------------------------------

// CLASS METHODS
inline int StatContextConfigurationChoice::maxSupportedBdexVersion()
{
    return 1;  // versions start at 1.
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
template <typename t_STREAM>
t_STREAM& StatContextConfigurationChoice::bdexStreamIn(t_STREAM& stream,
                                                       int       version)
{
    if (stream) {
        switch (version) {
        case 1: {
            short selectionId;
            stream.getInt16(selectionId);
            if (!stream) {
                return stream;
            }
            switch (selectionId) {
            case SELECTION_ID_ID: {
                makeId();
                bslx::InStreamFunctions::bdexStreamIn(stream,
                                                      d_id.object(),
                                                      1);
            } break;
            case SELECTION_ID_NAME: {
                makeName();
                bslx::InStreamFunctions::bdexStreamIn(stream,
                                                      d_name.object(),
                                                      1);
            } break;
            case SELECTION_ID_UNDEFINED: {
                reset();
            } break;
            default: stream.invalidate();
            }
        } break;
        default: {
            stream.invalidate();
        }
        }
    }
    return stream;
}

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
template <typename t_STREAM>
t_STREAM& StatContextConfigurationChoice::bdexStreamOut(t_STREAM& stream,
                                                        int version) const
{
    switch (version) {
    case 1: {
        stream.putInt16(d_selectionId);
        switch (d_selectionId) {
        case SELECTION_ID_ID: {
            bslx::OutStreamFunctions::bdexStreamOut(stream, d_id.object(), 1);
        } break;
        case SELECTION_ID_NAME: {
            bslx::OutStreamFunctions::bdexStreamOut(stream,
                                                    d_name.object(),
                                                    1);
        } break;
        default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        }
    } break;
    }
    return stream;
}

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
inline int StatContextConfigurationFlags::maxSupportedBdexVersion()
{
    return 1;  // versions start at 1
}

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

template <typename t_STREAM>
t_STREAM& StatContextConfigurationFlags::bdexStreamIn(
    t_STREAM&                             stream,
    StatContextConfigurationFlags::Value& value,
    int                                   version)
{
    switch (version) {
    case 1: {
        int readValue;
        stream.getInt32(readValue);
        if (stream) {
            if (fromInt(&value, readValue)) {
                stream.invalidate();  // bad value in stream
            }
        }
    } break;
    default: {
        stream.invalidate();  // unrecognized version number
    } break;
    }
    return stream;
}

template <typename t_STREAM>
t_STREAM& StatContextConfigurationFlags::bdexStreamOut(
    t_STREAM&                            stream,
    StatContextConfigurationFlags::Value value,
    int                                  version)
{
    switch (version) {
    case 1: {
        stream.putInt32(value);  // Write the value as an int
    } break;
    }
    return stream;
}

// ----------------------------
// class StatContextUpdateFlags
// ----------------------------

// CLASS METHODS
inline int StatContextUpdateFlags::maxSupportedBdexVersion()
{
    return 1;  // versions start at 1
}

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

template <typename t_STREAM>
t_STREAM&
StatContextUpdateFlags::bdexStreamIn(t_STREAM&                      stream,
                                     StatContextUpdateFlags::Value& value,
                                     int                            version)
{
    switch (version) {
    case 1: {
        int readValue;
        stream.getInt32(readValue);
        if (stream) {
            if (fromInt(&value, readValue)) {
                stream.invalidate();  // bad value in stream
            }
        }
    } break;
    default: {
        stream.invalidate();  // unrecognized version number
    } break;
    }
    return stream;
}

template <typename t_STREAM>
t_STREAM&
StatContextUpdateFlags::bdexStreamOut(t_STREAM&                     stream,
                                      StatContextUpdateFlags::Value value,
                                      int                           version)
{
    switch (version) {
    case 1: {
        stream.putInt32(value);  // Write the value as an int
    } break;
    }
    return stream;
}

// ---------------------
// class StatValueFields
// ---------------------

// CLASS METHODS
inline int StatValueFields::maxSupportedBdexVersion()
{
    return 1;  // versions start at 1
}

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

template <typename t_STREAM>
t_STREAM& StatValueFields::bdexStreamIn(t_STREAM&               stream,
                                        StatValueFields::Value& value,
                                        int                     version)
{
    switch (version) {
    case 1: {
        int readValue;
        stream.getInt32(readValue);
        if (stream) {
            if (fromInt(&value, readValue)) {
                stream.invalidate();  // bad value in stream
            }
        }
    } break;
    default: {
        stream.invalidate();  // unrecognized version number
    } break;
    }
    return stream;
}

template <typename t_STREAM>
t_STREAM& StatValueFields::bdexStreamOut(t_STREAM&              stream,
                                         StatValueFields::Value value,
                                         int                    version)
{
    switch (version) {
    case 1: {
        stream.putInt32(value);  // Write the value as an int
    } break;
    }
    return stream;
}

// -------------------
// class StatValueType
// -------------------

// CLASS METHODS
inline int StatValueType::maxSupportedBdexVersion()
{
    return 1;  // versions start at 1
}

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

template <typename t_STREAM>
t_STREAM& StatValueType::bdexStreamIn(t_STREAM&             stream,
                                      StatValueType::Value& value,
                                      int                   version)
{
    switch (version) {
    case 1: {
        int readValue;
        stream.getInt32(readValue);
        if (stream) {
            if (fromInt(&value, readValue)) {
                stream.invalidate();  // bad value in stream
            }
        }
    } break;
    default: {
        stream.invalidate();  // unrecognized version number
    } break;
    }
    return stream;
}

template <typename t_STREAM>
t_STREAM& StatValueType::bdexStreamOut(t_STREAM&            stream,
                                       StatValueType::Value value,
                                       int                  version)
{
    switch (version) {
    case 1: {
        stream.putInt32(value);  // Write the value as an int
    } break;
    }
    return stream;
}

// ---------------------
// class StatValueUpdate
// ---------------------

// CLASS METHODS
inline int StatValueUpdate::maxSupportedBdexVersion()
{
    return 1;  // versions start at 1.
}

// MANIPULATORS
template <typename t_STREAM>
t_STREAM& StatValueUpdate::bdexStreamIn(t_STREAM& stream, int version)
{
    if (stream) {
        switch (version) {
        case 1: {
            bslx::InStreamFunctions::bdexStreamIn(stream, d_fieldMask, 1);
            bslx::InStreamFunctions::bdexStreamIn(stream, d_fields, 1);
        } break;
        default: {
            stream.invalidate();
        }
        }
    }
    return stream;
}

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
template <typename t_STREAM>
t_STREAM& StatValueUpdate::bdexStreamOut(t_STREAM& stream, int version) const
{
    switch (version) {
    case 1: {
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->fieldMask(), 1);
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->fields(), 1);
    } break;
    }
    return stream;
}

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

// CLASS METHODS
inline int StatValueDefinition::maxSupportedBdexVersion()
{
    return 1;  // versions start at 1.
}

// MANIPULATORS
template <typename t_STREAM>
t_STREAM& StatValueDefinition::bdexStreamIn(t_STREAM& stream, int version)
{
    if (stream) {
        switch (version) {
        case 1: {
            bslx::InStreamFunctions::bdexStreamIn(stream, d_name, 1);
            StatValueType::bdexStreamIn(stream, d_type, 1);
            bslx::InStreamFunctions::bdexStreamIn(stream, d_historySizes, 1);
        } break;
        default: {
            stream.invalidate();
        }
        }
    }
    return stream;
}

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
template <typename t_STREAM>
t_STREAM& StatValueDefinition::bdexStreamOut(t_STREAM& stream,
                                             int       version) const
{
    switch (version) {
    case 1: {
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->name(), 1);
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->type(), 1);
        bslx::OutStreamFunctions::bdexStreamOut(stream,
                                                this->historySizes(),
                                                1);
    } break;
    }
    return stream;
}

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

// CLASS METHODS
inline int StatContextConfiguration::maxSupportedBdexVersion()
{
    return 1;  // versions start at 1.
}

// MANIPULATORS
template <typename t_STREAM>
t_STREAM& StatContextConfiguration::bdexStreamIn(t_STREAM& stream, int version)
{
    if (stream) {
        switch (version) {
        case 1: {
            bslx::InStreamFunctions::bdexStreamIn(stream, d_flags, 1);
            bslx::InStreamFunctions::bdexStreamIn(stream, d_choice, 1);
            bslx::InStreamFunctions::bdexStreamIn(stream, d_values, 1);
        } break;
        default: {
            stream.invalidate();
        }
        }
    }
    return stream;
}

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
template <typename t_STREAM>
t_STREAM& StatContextConfiguration::bdexStreamOut(t_STREAM& stream,
                                                  int       version) const
{
    switch (version) {
    case 1: {
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->flags(), 1);
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->choice(), 1);
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->values(), 1);
    } break;
    }
    return stream;
}

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

// CLASS METHODS
inline int StatContextUpdate::maxSupportedBdexVersion()
{
    return 1;  // versions start at 1.
}

// MANIPULATORS
template <typename t_STREAM>
t_STREAM& StatContextUpdate::bdexStreamIn(t_STREAM& stream, int version)
{
    if (stream) {
        switch (version) {
        case 1: {
            bslx::InStreamFunctions::bdexStreamIn(stream, d_id, 1);
            bslx::InStreamFunctions::bdexStreamIn(stream, d_flags, 1);
            bslx::InStreamFunctions::bdexStreamIn(stream, d_timeStamp, 1);
            bslx::InStreamFunctions::bdexStreamIn(stream, d_configuration, 1);
            bslx::InStreamFunctions::bdexStreamIn(stream, d_directValues, 1);
            bslx::InStreamFunctions::bdexStreamIn(stream, d_expiredValues, 1);
            bslx::InStreamFunctions::bdexStreamIn(stream, d_subcontexts, 1);
        } break;
        default: {
            stream.invalidate();
        }
        }
    }
    return stream;
}

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
template <typename t_STREAM>
t_STREAM& StatContextUpdate::bdexStreamOut(t_STREAM& stream, int version) const
{
    switch (version) {
    case 1: {
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->id(), 1);
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->flags(), 1);
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->timeStamp(), 1);
        bslx::OutStreamFunctions::bdexStreamOut(stream,
                                                this->configuration(),
                                                1);
        bslx::OutStreamFunctions::bdexStreamOut(stream,
                                                this->directValues(),
                                                1);
        bslx::OutStreamFunctions::bdexStreamOut(stream,
                                                this->expiredValues(),
                                                1);
        bslx::OutStreamFunctions::bdexStreamOut(stream,
                                                this->subcontexts(),
                                                1);
    } break;
    }
    return stream;
}

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
inline int StatContextUpdateList::maxSupportedBdexVersion()
{
    return 1;  // versions start at 1.
}

// MANIPULATORS
template <typename t_STREAM>
t_STREAM& StatContextUpdateList::bdexStreamIn(t_STREAM& stream, int version)
{
    if (stream) {
        switch (version) {
        case 1: {
            bslx::InStreamFunctions::bdexStreamIn(stream, d_contexts, 1);
        } break;
        default: {
            stream.invalidate();
        }
        }
    }
    return stream;
}

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
template <typename t_STREAM>
t_STREAM& StatContextUpdateList::bdexStreamOut(t_STREAM& stream,
                                               int       version) const
{
    switch (version) {
    case 1: {
        bslx::OutStreamFunctions::bdexStreamOut(stream, this->contexts(), 1);
    } break;
    }
    return stream;
}

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

inline bool
mwcstm::operator==(const mwcstm::StatContextConfigurationChoice& lhs,
                   const mwcstm::StatContextConfigurationChoice& rhs)
{
    typedef mwcstm::StatContextConfigurationChoice Class;
    if (lhs.selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_ID: return lhs.id() == rhs.id();
        case Class::SELECTION_ID_NAME: return lhs.name() == rhs.name();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

inline bool
mwcstm::operator!=(const mwcstm::StatContextConfigurationChoice& lhs,
                   const mwcstm::StatContextConfigurationChoice& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream&
mwcstm::operator<<(bsl::ostream&                                 stream,
                   const mwcstm::StatContextConfigurationChoice& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mwcstm::hashAppend(t_HASH_ALGORITHM&                             hashAlg,
                        const mwcstm::StatContextConfigurationChoice& object)
{
    typedef mwcstm::StatContextConfigurationChoice Class;
    using bslh::hashAppend;
    hashAppend(hashAlg, object.selectionId());
    switch (object.selectionId()) {
    case Class::SELECTION_ID_ID: hashAppend(hashAlg, object.id()); break;
    case Class::SELECTION_ID_NAME: hashAppend(hashAlg, object.name()); break;
    default:
        BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == object.selectionId());
    }
}

inline bsl::ostream&
mwcstm::operator<<(bsl::ostream&                                stream,
                   mwcstm::StatContextConfigurationFlags::Value rhs)
{
    return mwcstm::StatContextConfigurationFlags::print(stream, rhs);
}

inline bsl::ostream&
mwcstm::operator<<(bsl::ostream&                         stream,
                   mwcstm::StatContextUpdateFlags::Value rhs)
{
    return mwcstm::StatContextUpdateFlags::print(stream, rhs);
}

inline bsl::ostream& mwcstm::operator<<(bsl::ostream&                  stream,
                                        mwcstm::StatValueFields::Value rhs)
{
    return mwcstm::StatValueFields::print(stream, rhs);
}

inline bsl::ostream& mwcstm::operator<<(bsl::ostream&                stream,
                                        mwcstm::StatValueType::Value rhs)
{
    return mwcstm::StatValueType::print(stream, rhs);
}

inline bool mwcstm::operator==(const mwcstm::StatValueUpdate& lhs,
                               const mwcstm::StatValueUpdate& rhs)
{
    return lhs.fieldMask() == rhs.fieldMask() && lhs.fields() == rhs.fields();
}

inline bool mwcstm::operator!=(const mwcstm::StatValueUpdate& lhs,
                               const mwcstm::StatValueUpdate& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mwcstm::operator<<(bsl::ostream&                  stream,
                                        const mwcstm::StatValueUpdate& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mwcstm::hashAppend(t_HASH_ALGORITHM&              hashAlg,
                        const mwcstm::StatValueUpdate& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.fieldMask());
    hashAppend(hashAlg, object.fields());
}

inline bool mwcstm::operator==(const mwcstm::StatValueDefinition& lhs,
                               const mwcstm::StatValueDefinition& rhs)
{
    return lhs.name() == rhs.name() && lhs.type() == rhs.type() &&
           lhs.historySizes() == rhs.historySizes();
}

inline bool mwcstm::operator!=(const mwcstm::StatValueDefinition& lhs,
                               const mwcstm::StatValueDefinition& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mwcstm::operator<<(bsl::ostream& stream,
                                        const mwcstm::StatValueDefinition& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mwcstm::hashAppend(t_HASH_ALGORITHM&                  hashAlg,
                        const mwcstm::StatValueDefinition& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.name());
    hashAppend(hashAlg, object.type());
    hashAppend(hashAlg, object.historySizes());
}

inline bool mwcstm::operator==(const mwcstm::StatContextConfiguration& lhs,
                               const mwcstm::StatContextConfiguration& rhs)
{
    return lhs.flags() == rhs.flags() && lhs.choice() == rhs.choice() &&
           lhs.values() == rhs.values();
}

inline bool mwcstm::operator!=(const mwcstm::StatContextConfiguration& lhs,
                               const mwcstm::StatContextConfiguration& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream&
mwcstm::operator<<(bsl::ostream&                           stream,
                   const mwcstm::StatContextConfiguration& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mwcstm::hashAppend(t_HASH_ALGORITHM&                       hashAlg,
                        const mwcstm::StatContextConfiguration& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.flags());
    hashAppend(hashAlg, object.choice());
    hashAppend(hashAlg, object.values());
}

inline bool mwcstm::operator==(const mwcstm::StatContextUpdate& lhs,
                               const mwcstm::StatContextUpdate& rhs)
{
    return lhs.id() == rhs.id() && lhs.flags() == rhs.flags() &&
           lhs.timeStamp() == rhs.timeStamp() &&
           lhs.configuration() == rhs.configuration() &&
           lhs.directValues() == rhs.directValues() &&
           lhs.expiredValues() == rhs.expiredValues() &&
           lhs.subcontexts() == rhs.subcontexts();
}

inline bool mwcstm::operator!=(const mwcstm::StatContextUpdate& lhs,
                               const mwcstm::StatContextUpdate& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream& mwcstm::operator<<(bsl::ostream& stream,
                                        const mwcstm::StatContextUpdate& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mwcstm::hashAppend(t_HASH_ALGORITHM&                hashAlg,
                        const mwcstm::StatContextUpdate& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.id());
    hashAppend(hashAlg, object.flags());
    hashAppend(hashAlg, object.timeStamp());
    hashAppend(hashAlg, object.configuration());
    hashAppend(hashAlg, object.directValues());
    hashAppend(hashAlg, object.expiredValues());
    hashAppend(hashAlg, object.subcontexts());
}

inline bool mwcstm::operator==(const mwcstm::StatContextUpdateList& lhs,
                               const mwcstm::StatContextUpdateList& rhs)
{
    return lhs.contexts() == rhs.contexts();
}

inline bool mwcstm::operator!=(const mwcstm::StatContextUpdateList& lhs,
                               const mwcstm::StatContextUpdateList& rhs)
{
    return !(lhs == rhs);
}

inline bsl::ostream&
mwcstm::operator<<(bsl::ostream&                        stream,
                   const mwcstm::StatContextUpdateList& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void mwcstm::hashAppend(t_HASH_ALGORITHM&                    hashAlg,
                        const mwcstm::StatContextUpdateList& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.contexts());
}

}  // close enterprise namespace
#endif

// GENERATED BY BLP_BAS_CODEGEN_2023.02.18