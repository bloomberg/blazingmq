// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqvt_valueorerror.h -*-C++-*-
#ifndef INCLUDED_BMQT_VALUEORERROR
#define INCLUDED_BMQT_VALUEORERROR

//@PURPOSE: Provide a value-semantic class that may hold a value or an error.
//
//@CLASSES:
//  bmqvt::ValueOrError: Represents a Value or Error type.
//
//@DESCRIPTION: The 'bmqvt::ValueOrError' provides a type that abstracts a wide
// range of function return values in the presence of errors.  It might be
// uninitialized, initialized with an error or initialized with a value.  Both
// error and value might be arbitrary types, defined as template arguments
// 'ERROR' and 'VALUE' respectively.  To better understand the reasoning behind
// it, read https://goo.gl/RsA2Dd by Bartosz Milewski.
//
// The starting point was an object with a single 'choice' created with
// bas_codegen.  Then converted it to generic type, used
// 'bslma::ConstructionUtil' to do placement new and slightly altered other
// functions.
//
// 'bmqvt::ValueOrError' aims to be used in the following way:
//
//..
//  struct ErrorsForFoo {
//      enum Type {
//          DidntFindDisk,
//          ServerUnplugged
//      };
//  };
//
//  void foo(ValueOrError<SuccessType, ErrorsForFoo> *out,
//           const bslstl::StringRef                  arg);
//
//  // Use case:
//
//  ValueOrError<SuccessType, ErrorsForFoo> fooResult;
//  foo(&fooResult, "test");
//
//  if (fooResult.isError()) {
//      switch (fooResult.error()) {
//          case DidntFindDisk:
//              BALL_LOG_ERROR << "No Disk";
//          break;
//          case ServerUnplugged:
//              BALL_LOG_ERROR << "No Server";
//          break;
//          default:
//              BSLS_ASSERT_OPT(false && "Unknown error");
//      }
//      return;
//  }
//
//  // Success case
//  const SuccessType& result = fooResult.value();
//
//  // ... do something
//..

// BDE
#include <bdlb_print.h>
#include <bdlb_variant.h>
#include <bsl_ostream.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_assert.h>
#include <bslmf_issame.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>

namespace BloombergLP {

namespace bmqvt {

// ==================
// class ValueOrError
// ==================

template <class VALUE, class ERROR>
class ValueOrError {
    // DATA

    // The internal state of this object.
    bdlb::Variant2<VALUE, ERROR> d_variant;

  public:
    // TYPES

    /// `ValueType` is an alias for the underlying `VALUE` upon which this
    /// template class is parameterized, and represents the type a
    /// `ValueOrError` object represents when it holds a value.
    typedef VALUE ValueType;

    /// `ErrorType` is an alias for the underlying `ERROR` upon which this
    /// template class is parameterized, and represents the type a
    /// `ValueOrError` object represents when it holds an error.
    typedef ERROR ErrorType;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ValueOrError, bslma::UsesBslmaAllocator)

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
    static bsl::ostream& print(bsl::ostream&                     stream,
                               const ValueOrError<VALUE, ERROR>& value,
                               int                               level,
                               int spacesPerLevel);

    // CREATORS

    /// Create an object of type `ValueOrError` having the `Undefined`
    /// state.  Use the optionally specified `basicAllocator` to supply
    /// memory.  If `basicAllocator` is 0, the currently installed default
    /// allocator is used.
    explicit ValueOrError(bslma::Allocator* basicAllocator = 0);

    /// Create an object of type `ValueOrError` having the value of the
    /// specified `original` object.  Use the optionally specified
    /// `basicAllocator` to supply memory.  If `basicAllocator` is 0, the
    /// currently installed default allocator is used.
    ValueOrError(const ValueOrError& original,
                 bslma::Allocator*   basicAllocator = 0);

    /// Destroy this object.
    ~ValueOrError();

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ValueOrError& operator=(const ValueOrError& rhs);

    /// Reset this object to the default value (i.e., its value upon default
    /// construction).
    void reset();

    /// Set the value of this object to be a "Value" value.  Optionally
    /// specify the `value` of the "Value".  If `value` is not specified,
    /// the default "Value" value is used.
    VALUE& makeValue();
    VALUE& makeValue(const VALUE& value);

    /// Set the value of this object to be a specified `error` value.
    /// Optionally specify the `value` of the `error`.  If `value` is not
    /// specified, the default `error` value is used.
    ERROR& makeError();
    ERROR& makeError(const ERROR& error);

    /// Return a reference to the modifiable "Value" selection of this
    /// object if "Value" is the current selection.  The behavior is
    /// undefined unless "Value" is the selection of this object.
    VALUE& value();

    /// Return a reference to the modifiable "Error" selection of this
    /// object if "Error" is the current selection.  The behavior is
    /// undefined unless "Error" is the selection of this object.
    ERROR& error();

    // ACCESSORS

    /// Return a reference to the non-modifiable "Value" selection of this
    /// object if "Value" is the current selection.  The behavior is
    /// undefined unless "Value" is the selection of this object.
    const VALUE& value() const;

    /// Return a reference to the non-modifiable "Error" selection of this
    /// object if "Error" is the current selection.  The behavior is
    /// undefined unless "Error" is the selection of this object.
    const ERROR& error() const;

    /// Return the value of the underlying `ValueType` if `isValue()` is
    /// `true`, or the specified `defaultValue` otherwise (including if the
    /// value is undefined).
    const VALUE& valueOr(const VALUE& defaultValue) const;

    /// Loads into the specified `out` a `ValueOrError` with the specified
    /// `TARGET_VALUE` as `ValueType` and the same `ErrorType`.  The
    /// specified `f` functor is required to be able to convert `VALUE` to
    /// `TARGET_VALUE`.  Returns a reference to the `out` pointee.
    template <class TARGET_VALUE, class UNARYOPERATION>
    ValueOrError<TARGET_VALUE, ERROR>&
    mapValue(ValueOrError<TARGET_VALUE, ERROR>* out,
             const UNARYOPERATION&              f) const;

    /// Loads into the specified `out` a `ValueOrError` with the specified
    /// `TARGET_ERROR` as `ErrorType` and the same `ValueType`.  The
    /// specified `f` functor is required to be able to convert `ERROR` to
    /// `TARGET_ERROR`.  Returns a reference to the `out` pointee.
    template <class TARGET_ERROR, class UNARYOPERATION>
    ValueOrError<VALUE, TARGET_ERROR>&
    mapError(ValueOrError<VALUE, TARGET_ERROR>* out,
             const UNARYOPERATION&              f) const;

    /// Loads into the specified `out` a `ValueOrError` with the specified
    /// `TARGET_VALUE` as `ValueType` and the specified `TARGET_ERROR` as
    /// `ErrorType`.  The specified `f` functor is required to be able to
    /// convert `VALUE` to `TARGET_VALUE` and the specified `g` functor is
    /// required to be able to convert `ERROR` to `TARGET_ERROR`.  Returns a
    /// reference to the `out` pointee.
    template <class TARGET_VALUE,
              class TARGET_ERROR,
              class VALUE_UNARYOPERATION,
              class ERROR_UNARYOPERATION>
    ValueOrError<TARGET_VALUE, TARGET_ERROR>&
    map(ValueOrError<TARGET_VALUE, TARGET_ERROR>* out,
        const VALUE_UNARYOPERATION&               f,
        const ERROR_UNARYOPERATION&               g) const;

    /// Return `true` if the value of this object is a "Value" value, and
    /// return `false` otherwise.
    bool isValue() const;

    /// Return `true` if the value of this object is a "Error" value, and
    /// return `false` otherwise.
    bool isError() const;

    /// Return `true` if the value of this object is undefined, and `false`
    /// otherwise.
    bool isUndefined() const;
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
template <class VALUE, class ERROR>
inline bsl::ostream& operator<<(bsl::ostream&                     stream,
                                const ValueOrError<VALUE, ERROR>& value);

// ============================================================================
//                         INLINE FUNCTION DEFINITIONS
// ============================================================================

// ------------------
// class ValueOrError
// ------------------

// CREATORS
template <class VALUE, class ERROR>
inline ValueOrError<VALUE, ERROR>::ValueOrError(
    bslma::Allocator* basicAllocator)
: d_variant(basicAllocator)
{
    BSLMF_ASSERT((!bsl::is_same<VALUE, ERROR>::value));
    // An assertion that fails if the specified 'VALUE' and 'ERROR' types
    // are the same.
    // NOTE: If you get 'static assertion failed: (!bsl::is_same< VALUE,
    //       ERROR >::value)'. We use types on 'bdlb::Variant2' to
    //       distinguish between two different *values*; the one provided
    //       by 'value()' and the one provided by 'error()'.  In order to
    //       do so, we reasonably assume that those two types are distinct.
    //       If this isn't the case, please use appropriate wrappers.  We
    //       do want to use 'bdlb::Variant2' because it is and we expect
    //       it to continue to be as optimized as possible regarding
    //       low-level allocator usage, move semantics etc.
}

template <class VALUE, class ERROR>
ValueOrError<VALUE, ERROR>::ValueOrError(
    const ValueOrError<VALUE, ERROR>& original,
    bslma::Allocator*                 basicAllocator)
: d_variant(original.d_variant, basicAllocator)
{
    BSLMF_ASSERT((!bsl::is_same<VALUE, ERROR>::value));
    // See default constructor.
}

template <class VALUE, class ERROR>
inline ValueOrError<VALUE, ERROR>::~ValueOrError()
{
    BSLMF_ASSERT((!bsl::is_same<VALUE, ERROR>::value));

    reset();
}

template <class VALUE, class ERROR>
inline VALUE& ValueOrError<VALUE, ERROR>::value()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_variant.template is<VALUE>());

    return d_variant.template the<VALUE>();
}

template <class VALUE, class ERROR>
inline ERROR& ValueOrError<VALUE, ERROR>::error()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_variant.template is<ERROR>());

    return d_variant.template the<ERROR>();
}

template <class VALUE, class ERROR>
inline const VALUE&
ValueOrError<VALUE, ERROR>::valueOr(const VALUE& defaultValue) const
{
    return isValue() ? value() : defaultValue;
}

template <class VALUE, class ERROR>
template <class TARGET_VALUE, class UNARYOPERATION>
ValueOrError<TARGET_VALUE, ERROR>&
ValueOrError<VALUE, ERROR>::mapValue(ValueOrError<TARGET_VALUE, ERROR>* out,
                                     const UNARYOPERATION& f) const
{
    if (isValue()) {
        out->makeValue(f(value()));
    }
    else if (isError()) {
        out->makeError(error());
    }
    return *out;
}

template <class VALUE, class ERROR>
template <class TARGET_ERROR, class UNARYOPERATION>
ValueOrError<VALUE, TARGET_ERROR>&
ValueOrError<VALUE, ERROR>::mapError(ValueOrError<VALUE, TARGET_ERROR>* out,
                                     const UNARYOPERATION& f) const
{
    if (isValue()) {
        out->makeValue(value());
    }
    else if (isError()) {
        out->makeError(f(error()));
    }
    return *out;
}

template <class VALUE, class ERROR>
template <class TARGET_VALUE,
          class TARGET_ERROR,
          class VALUE_UNARYOPERATION,
          class ERROR_UNARYOPERATION>
ValueOrError<TARGET_VALUE, TARGET_ERROR>&
ValueOrError<VALUE, ERROR>::map(ValueOrError<TARGET_VALUE, TARGET_ERROR>* out,
                                const VALUE_UNARYOPERATION&               f,
                                const ERROR_UNARYOPERATION& g) const
{
    if (isValue()) {
        out->makeValue(f(value()));
    }
    else if (isError()) {
        out->makeError(g(error()));
    }
    return *out;
}

template <class VALUE, class ERROR>
inline const VALUE& ValueOrError<VALUE, ERROR>::value() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_variant.template is<VALUE>());

    return d_variant.template the<VALUE>();
}

template <class VALUE, class ERROR>
inline const ERROR& ValueOrError<VALUE, ERROR>::error() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_variant.template is<ERROR>());

    return d_variant.template the<ERROR>();
}

template <class VALUE, class ERROR>
inline bool ValueOrError<VALUE, ERROR>::isValue() const
{
    return d_variant.template is<VALUE>();
}

template <class VALUE, class ERROR>
inline bool ValueOrError<VALUE, ERROR>::isError() const
{
    return d_variant.template is<ERROR>();
}

template <class VALUE, class ERROR>
inline bool ValueOrError<VALUE, ERROR>::isUndefined() const
{
    return d_variant.isUnset();
}

// MANIPULATORS

template <class VALUE, class ERROR>
ValueOrError<VALUE, ERROR>&
ValueOrError<VALUE, ERROR>::operator=(const ValueOrError<VALUE, ERROR>& rhs)
{
    d_variant = rhs.d_variant;

    return *this;
}

template <class VALUE, class ERROR>
void ValueOrError<VALUE, ERROR>::reset()
{
    d_variant.reset();
}

template <class VALUE, class ERROR>
VALUE& ValueOrError<VALUE, ERROR>::makeValue()
{
    d_variant.template createInPlace<VALUE>();
    return ValueOrError<VALUE, ERROR>::value();
}

template <class VALUE, class ERROR>
VALUE& ValueOrError<VALUE, ERROR>::makeValue(const VALUE& value)
{
    d_variant.template createInPlace<VALUE>(value);
    return ValueOrError<VALUE, ERROR>::value();
}

template <class VALUE, class ERROR>
ERROR& ValueOrError<VALUE, ERROR>::makeError()
{
    d_variant.template createInPlace<ERROR>();
    return ValueOrError<VALUE, ERROR>::error();
}

template <class VALUE, class ERROR>
ERROR& ValueOrError<VALUE, ERROR>::makeError(const ERROR& error)
{
    d_variant.template createInPlace<ERROR>(error);
    return ValueOrError<VALUE, ERROR>::error();
}

// ACCESSORS

template <class VALUE, class ERROR>
bsl::ostream&
ValueOrError<VALUE, ERROR>::print(bsl::ostream&                     stream,
                                  const ValueOrError<VALUE, ERROR>& value,
                                  int                               level,
                                  int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);

    if (value.isUndefined()) {
        stream << "[ UNDEFINED ]";
    }
    else if (value.isValue()) {
        stream << "[ value = " << value.d_variant << " ]";
    }
    else {
        stream << "[ error = " << value.d_variant << " ]";
    }

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

}  // close package namespace

// FREE FUNCTIONS

template <class VALUE, class ERROR>
inline bsl::ostream&
bmqvt::operator<<(bsl::ostream&                            stream,
                  const bmqvt::ValueOrError<VALUE, ERROR>& value)
{
    return ValueOrError<VALUE, ERROR>::print(stream, value, 0, -1);
}

}  // close enterprise namespace
#endif
