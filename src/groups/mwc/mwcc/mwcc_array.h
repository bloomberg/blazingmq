// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mwcc_array.h                                                       -*-C++-*-
#ifndef INCLUDED_MWCC_ARRAY
#define INCLUDED_MWCC_ARRAY

//@PURPOSE: Provide a hybrid of static and dynamic array.
//
//@CLASSES:
//  mwcc::ArraySpan: Non-owning view over an array.
//  mwcc::Array: Hybrid of static and dynamic array.
//
//@SEE_ALSO: bsl::vector
//
//@DESCRIPTION: 'mwcc::Array' provides a container which can be seen as a
// hybrid of static and dynamic array (aka, 'vector').  User can declare length
// of the 'static part' of the array at compile time, and array can grow
// dynamically at runtime if required.  An advantage of this hybrid approach is
// that if user has a good intuition about the maximum number of elements that
// this container will hold, user can provide an appropriate value for the
// 'static part' of the array at compile-time, thereby preventing any runtime
// memory allocation in the container.  Note that this optimization comes at
// the cost of additional memory, which is directly proportional to the value
// of static length provided at compile-time.
//
/// Exception Safety
///----------------
// At this time, this component provides *no* exception safety guarantee.  In
// other words, this component is *not* exception neutral.  If any exception is
// thrown during the invocation of a method on the object, the object is left
// in an inconsistent state, and using the object from that point forward will
// cause undefined behavior.
//
/// TBD:
///----
//: o Documentation
//: o Tests (including perf/bench)

// MWC

// BDE
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_vector.h>
#include <bslalg_arraydestructionprimitives.h>
#include <bslalg_arrayprimitives.h>
#include <bslma_allocator.h>
#include <bslma_allocatortraits.h>
#include <bslma_stdallocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_objectbuffer.h>

namespace BloombergLP {

namespace mwcc {

// FORWARD DECLARATION
template <class TYPE, size_t STATIC_LEN>
class Array;

// ===============
// class ArraySpan
// ===============

/// Non-owning view over an Array.
template <class VALUE>
class ArraySpan {
  private:
    // PRIVATE TYPES
    typedef VALUE*       internal_iterator;
    typedef const VALUE* const_internal_iterator;

    // FRIENDS
    template <class TYPE, size_t LEN>
    friend class Array;

    // DATA
    VALUE* d_begin_p;
    VALUE* d_end_p;

    // PRIVATE MANIPULATORS
    void setEnd(VALUE* end);

  public:
    // TYPES
    typedef VALUE          value_type;
    typedef size_t         size_type;
    typedef bsl::ptrdiff_t difference_type;
    typedef VALUE&         reference;
    typedef const VALUE&   const_reference;

    typedef internal_iterator       iterator;
    typedef const_internal_iterator const_iterator;

    // CREATORS
    ArraySpan();

    ArraySpan(VALUE* first, VALUE* last);

    // MANIPULATORS
    VALUE& operator[](size_t index);

    VALUE& front();

    VALUE& back();

    iterator begin();

    iterator end();

    void reset(VALUE* begin, VALUE* end);

    // ACCESSORS
    const VALUE& operator[](size_t index) const;

    bool empty() const;

    size_t size() const;

    const VALUE& front() const;

    const VALUE& back() const;

    const_iterator begin() const;

    const_iterator end() const;
};

// ===========
// class Array
// ===========

template <class TYPE, size_t STATIC_LEN>
class Array {
  private:
    // PRIVATE TYPES
    typedef ArraySpan<TYPE>                              Span;
    typedef bsl::allocator_traits<bsl::allocator<TYPE> > AllocTraits;

    struct SmallRepr {
        bsls::ObjectBuffer<TYPE> d_staticArray[STATIC_LEN];
    };

    struct LargeRepr {
        size_t d_cap;
    };

    // DATA
    union {
        bsls::ObjectBuffer<SmallRepr> d_smallRepr;
        bsls::ObjectBuffer<LargeRepr> d_largeRepr;
    };
    ArraySpan<TYPE>      d_span;
    bsl::allocator<TYPE> d_allocator;

    // PRIVATE ACCESSORS
    bool isSmall() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Array, bslma::UsesBslmaAllocator)
    static const size_t static_size = STATIC_LEN;

    // TYPES
    typedef bsl::allocator<TYPE>           allocator_type;
    typedef typename Span::value_type      value_type;
    typedef size_t                         size_type;
    typedef bsl::ptrdiff_t                 difference_type;
    typedef typename Span::reference       reference;
    typedef typename Span::const_reference const_reference;

    typedef typename Span::iterator       iterator;
    typedef typename Span::const_iterator const_iterator;

    // CREATORS
    explicit Array(const allocator_type& basicAllocator = allocator_type());

    explicit Array(size_t                count,
                   const TYPE&           val            = TYPE(),
                   const allocator_type& basicAllocator = allocator_type());

    template <class INPUT_ITER>
    Array(INPUT_ITER            first,
          INPUT_ITER            last,
          const allocator_type& basicAllocator = allocator_type());

    Array(const Array&          other,
          const allocator_type& basicAllocator = allocator_type());

    /// Copy from an `Array` having a different length.  Note that the
    /// weird spelling for the default argument is a workaround for an XlC
    /// bug whereby it fails to find names in the parent template.
    template <size_t LEN>
    Array(const Array<TYPE, LEN>& other,
          const allocator_type&   basicAllocator = bsl::allocator<char>());

    ~Array();

    // MANIPULATORS
    Array& operator=(const Array& rhs);

    template <size_t LEN>
    Array& operator=(const Array<TYPE, LEN>& rhs);

    void clear();

    void resize(size_t size, const TYPE& val = TYPE());

    void reserve(size_t n);

    template <class INPUT_ITER>
    void assign(INPUT_ITER first, INPUT_ITER last);

    void push_back(const TYPE& value);

    TYPE& operator[](size_t index);

    TYPE& front();

    TYPE& back();

    iterator begin();

    iterator end();

    // ACCESSORS
    allocator_type get_allocator() const;

    size_t capacity() const;

    const TYPE& operator[](size_t index) const;

    bool empty() const;

    size_t size() const;

    const TYPE& front() const;

    const TYPE& back() const;

    const_iterator begin() const;

    const_iterator end() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// class ArraySpan
// ----------------

// PRIVATE MANIPULATORS
template <class VALUE>
inline void ArraySpan<VALUE>::setEnd(VALUE* end)
{
    BSLS_ASSERT_SAFE(d_begin_p <= d_end_p);
    BSLS_ASSERT_SAFE(d_begin_p <= end);
    d_end_p = end;
}

// CREATORS
template <class VALUE>
inline ArraySpan<VALUE>::ArraySpan()
: d_begin_p(0)
, d_end_p(0)
{
}

template <class VALUE>
inline ArraySpan<VALUE>::ArraySpan(VALUE* b, VALUE* e)
: d_begin_p(b)
, d_end_p(e)
{
    BSLS_ASSERT_SAFE(d_begin_p <= d_end_p);
}

// MANIPULATORS
template <class VALUE>
inline VALUE& ArraySpan<VALUE>::operator[](size_t index)
{
    BSLS_ASSERT_SAFE(d_begin_p < d_end_p);
    BSLS_ASSERT_SAFE(index < size());
    return d_begin_p[index];
}

template <class VALUE>
inline VALUE& ArraySpan<VALUE>::front()
{
    BSLS_ASSERT_SAFE(d_begin_p < d_end_p);
    return *d_begin_p;
}

template <class VALUE>
inline VALUE& ArraySpan<VALUE>::back()
{
    BSLS_ASSERT_SAFE(d_begin_p < d_end_p);
    return *(d_end_p - 1);
}

template <class VALUE>
inline typename ArraySpan<VALUE>::iterator ArraySpan<VALUE>::begin()
{
    return d_begin_p;
}

template <class VALUE>
inline typename ArraySpan<VALUE>::iterator ArraySpan<VALUE>::end()
{
    return d_end_p;
}

template <class VALUE>
inline void ArraySpan<VALUE>::reset(VALUE* begin, VALUE* end)
{
    BSLS_ASSERT_SAFE(begin <= end);
    d_begin_p = begin;
    d_end_p   = end;
}

// ACCESSORS
template <class VALUE>
inline const VALUE& ArraySpan<VALUE>::operator[](size_t index) const
{
    BSLS_ASSERT_SAFE(d_begin_p < d_end_p);
    BSLS_ASSERT_SAFE(index < size());
    return d_begin_p[index];
}

template <class VALUE>
inline bool ArraySpan<VALUE>::empty() const
{
    return d_begin_p == d_end_p;
}

template <class VALUE>
inline size_t ArraySpan<VALUE>::size() const
{
    return bsl::distance(d_begin_p, d_end_p);
}

template <class VALUE>
inline const VALUE& ArraySpan<VALUE>::front() const
{
    BSLS_ASSERT_SAFE(d_begin_p < d_end_p);
    return *d_begin_p;
}

template <class VALUE>
inline const VALUE& ArraySpan<VALUE>::back() const
{
    BSLS_ASSERT_SAFE(d_begin_p < d_end_p);
    return *(d_end_p - 1);
}

template <class VALUE>
inline typename ArraySpan<VALUE>::const_iterator
ArraySpan<VALUE>::begin() const
{
    return d_begin_p;
}

template <class VALUE>
inline typename ArraySpan<VALUE>::const_iterator ArraySpan<VALUE>::end() const
{
    return d_end_p;
}

// -----------
// class Array
// -----------

// PRIVATE MANIPULATORS
template <class TYPE, size_t STATIC_LEN>
inline bool Array<TYPE, STATIC_LEN>::isSmall() const
{
    return d_span.begin() == &d_smallRepr.object().d_staticArray[0].object();
}

// CREATORS
template <class TYPE, size_t STATIC_LEN>
inline Array<TYPE, STATIC_LEN>::Array(const allocator_type& basicAllocator)
: d_smallRepr()
, d_span(&d_smallRepr.object().d_staticArray[0].object(),
         &d_smallRepr.object().d_staticArray[0].object())
, d_allocator(basicAllocator)
{
}

template <class TYPE, size_t STATIC_LEN>
inline Array<TYPE, STATIC_LEN>::Array(size_t                count,
                                      const TYPE&           val,
                                      const allocator_type& basicAllocator)
: d_smallRepr()
, d_span(&d_smallRepr.object().d_staticArray[0].object(),
         &d_smallRepr.object().d_staticArray[0].object())
, d_allocator(basicAllocator)
{
    resize(count, val);
}

template <class TYPE, size_t STATIC_LEN>
template <class INPUT_ITER>
inline Array<TYPE, STATIC_LEN>::Array(INPUT_ITER            first,
                                      INPUT_ITER            last,
                                      const allocator_type& basicAllocator)
: d_smallRepr()
, d_span(&d_smallRepr.object().d_staticArray[0].object(),
         &d_smallRepr.object().d_staticArray[0].object())
, d_allocator(basicAllocator)
{
    BSLS_ASSERT_SAFE(first < last);
    assign(first, last);
}

template <class TYPE, size_t STATIC_LEN>
inline Array<TYPE, STATIC_LEN>::Array(const Array&          other,
                                      const allocator_type& basicAllocator)
: d_smallRepr()
, d_span(&d_smallRepr.object().d_staticArray[0].object(),
         &d_smallRepr.object().d_staticArray[0].object())
, d_allocator(basicAllocator)
{
    assign(other.begin(), other.end());
}

template <class TYPE, size_t STATIC_LEN>
template <size_t LEN>
inline Array<TYPE, STATIC_LEN>::Array(const Array<TYPE, LEN>& other,
                                      const allocator_type&   basicAllocator)
: d_smallRepr()
, d_span(&d_smallRepr.object().d_staticArray[0].object(),
         &d_smallRepr.object().d_staticArray[0].object())
, d_allocator(basicAllocator)
{
    assign(other.begin(), other.end());
}

template <class TYPE, size_t STATIC_LEN>
inline Array<TYPE, STATIC_LEN>::~Array()
{
    clear();
    if (!isSmall()) {
        AllocTraits::deallocate(d_allocator, d_span.begin(), this->capacity());
    }
}

// MANIPULATORS
template <class TYPE, size_t STATIC_LEN>
inline Array<TYPE, STATIC_LEN>&
Array<TYPE, STATIC_LEN>::operator=(const Array& rhs)
{
    if (this == &rhs) {
        return *this;  // RETURN
    }

    assign(rhs.begin(), rhs.end());
    return *this;
}

template <class TYPE, size_t STATIC_LEN>
template <size_t LEN>
inline Array<TYPE, STATIC_LEN>&
Array<TYPE, STATIC_LEN>::operator=(const Array<TYPE, LEN>& rhs)
{
    assign(rhs.begin(), rhs.end());
    return *this;
}

template <class TYPE, size_t STATIC_LEN>
inline void Array<TYPE, STATIC_LEN>::clear()
{
    bslalg::ArrayDestructionPrimitives::destroy(d_span.begin(),
                                                d_span.end(),
                                                d_allocator);
    d_span.setEnd(d_span.begin());
}

template <class TYPE, size_t STATIC_LEN>
inline void Array<TYPE, STATIC_LEN>::resize(size_t sz, const TYPE& val)
{
    const size_t currSize = d_span.size();

    if (sz <= currSize) {
        bslalg::ArrayDestructionPrimitives::destroy(d_span.begin() + sz,
                                                    d_span.end(),
                                                    d_allocator);
        d_span.setEnd(d_span.begin() + sz);
        return;  // RETURN
    }

    reserve(sz);

    iterator       begIt = d_span.begin() + d_span.size();
    const iterator endIt = d_span.begin() + sz;
    while (begIt != endIt) {
        AllocTraits::construct(d_allocator, begIt++, val);
    }

    d_span.setEnd(endIt);
}

template <class TYPE, size_t STATIC_LEN>
inline void Array<TYPE, STATIC_LEN>::reserve(size_t n)
{
    const bool   small = isSmall();
    const size_t cap   = small ? STATIC_LEN : d_largeRepr.object().d_cap;
    if (n <= cap) {
        return;  // RETURN
    }

    // Since we're growing, new state *must* be dynamic (large repr).
    TYPE* arr = AllocTraits::allocate(d_allocator, n);

    bslalg::ArrayPrimitives::copyConstruct(arr,
                                           d_span.begin(),
                                           d_span.end(),
                                           d_allocator);

    const size_t sz = size();

    clear();
    if (!small) {
        AllocTraits::deallocate(d_allocator, d_span.begin(), cap);
    }

    d_span.reset(arr, arr + sz);
    d_largeRepr.object().d_cap = n;
}

template <class TYPE, size_t STATIC_LEN>
template <class INPUT_ITER>
inline void Array<TYPE, STATIC_LEN>::assign(INPUT_ITER first, INPUT_ITER last)
{
    const size_t sz = bsl::distance(first, last);
    clear();
    reserve(sz);

    iterator arrEnd = d_span.begin();
    while (first != last) {
        AllocTraits::construct(d_allocator, arrEnd++, *(first++));
    }

    d_span.setEnd(arrEnd);
}

template <class TYPE, size_t STATIC_LEN>
inline void Array<TYPE, STATIC_LEN>::push_back(const TYPE& value)
{
    const bool   small = isSmall();
    const size_t cap   = small ? STATIC_LEN : d_largeRepr.object().d_cap;
    const size_t sz    = d_span.size();

    BSLS_ASSERT_SAFE(sz <= cap);

    if (sz + 1 > cap) {
        const size_t newCap = bsl::Vector_Util::computeNewCapacity(
            sz + 1,
            cap,
            AllocTraits::max_size(d_allocator));

        TYPE* arr = AllocTraits::allocate(d_allocator, newCap);

        bslalg::ArrayPrimitives::copyConstruct(arr,
                                               d_span.begin(),
                                               d_span.end(),
                                               d_allocator);
        AllocTraits::construct(d_allocator, arr + sz, value);

        clear();
        if (!small) {
            AllocTraits::deallocate(d_allocator, d_span.begin(), cap);
        }

        d_span.reset(arr, arr + sz + 1);
        d_largeRepr.object().d_cap = newCap;
    }
    else {
        AllocTraits::construct(d_allocator, d_span.end(), value);
        d_span.setEnd(d_span.end() + 1);
    }
}

template <class TYPE, size_t STATIC_LEN>
inline TYPE& Array<TYPE, STATIC_LEN>::operator[](size_t index)
{
    return d_span[index];
}

template <class TYPE, size_t STATIC_LEN>
inline TYPE& Array<TYPE, STATIC_LEN>::front()
{
    return d_span.front();
}

template <class TYPE, size_t STATIC_LEN>
inline TYPE& Array<TYPE, STATIC_LEN>::back()
{
    return d_span.back();
}

template <class TYPE, size_t STATIC_LEN>
inline typename Array<TYPE, STATIC_LEN>::iterator
Array<TYPE, STATIC_LEN>::begin()
{
    return d_span.begin();
}

template <class TYPE, size_t STATIC_LEN>
inline typename Array<TYPE, STATIC_LEN>::iterator
Array<TYPE, STATIC_LEN>::end()
{
    return d_span.end();
}

// ACCESSORS
template <class TYPE, size_t STATIC_LEN>
inline typename Array<TYPE, STATIC_LEN>::allocator_type
Array<TYPE, STATIC_LEN>::get_allocator() const
{
    return d_allocator;
}

template <class TYPE, size_t STATIC_LEN>
inline size_t Array<TYPE, STATIC_LEN>::capacity() const
{
    return isSmall() ? STATIC_LEN : d_largeRepr.object().d_cap;
}

template <class TYPE, size_t STATIC_LEN>
inline const TYPE& Array<TYPE, STATIC_LEN>::operator[](size_t index) const
{
    return d_span[index];
}

template <class TYPE, size_t STATIC_LEN>
inline bool Array<TYPE, STATIC_LEN>::empty() const
{
    return d_span.empty();
}

template <class TYPE, size_t STATIC_LEN>
inline size_t Array<TYPE, STATIC_LEN>::size() const
{
    return d_span.size();
}

template <class TYPE, size_t STATIC_LEN>
inline const TYPE& Array<TYPE, STATIC_LEN>::front() const
{
    return d_span.front();
}

template <class TYPE, size_t STATIC_LEN>
inline const TYPE& Array<TYPE, STATIC_LEN>::back() const
{
    return d_span.back();
}

template <class TYPE, size_t STATIC_LEN>
inline typename Array<TYPE, STATIC_LEN>::const_iterator
Array<TYPE, STATIC_LEN>::begin() const
{
    return d_span.begin();
}

template <class TYPE, size_t STATIC_LEN>
inline typename Array<TYPE, STATIC_LEN>::const_iterator
Array<TYPE, STATIC_LEN>::end() const
{
    return d_span.end();
}

}  // close package namespace
}  // close enterprise namespace

#endif
