// Copyright 2022-2023 Bloomberg Finance L.P.
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

// bmqst_printutil.h -*-C++-*-
#ifndef INCLUDED_BMQST_PRINTUTIL
#define INCLUDED_BMQST_PRINTUTIL

//@PURPOSE: Provide utilities for printing things
//
//@CLASSES:
// bmqst::PrintUtil
//
//@DESCRIPTION:
// This component provides a set of utility functions useful for printing
// various objects.
//

#include <bdlb_print.h>
#include <bsl_map.h>
#include <bsl_ostream.h>
#include <bsl_set.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bsls {
class Stopwatch;
}

namespace bmqst {

// FORWARD DECLARATIONS
class TableInfoProvider;
template <typename TYPE>
class Printer;

// ================
// struct PrintUtil
// ================

struct PrintUtil {
    // CLASS METHODS

    /// Return the number of characters needed to print the specified
    /// `value`.
    static int printedValueLength(bsls::Types::Int64 value);

    /// Return the number of characters needed to print the specified
    /// `value` with the specified `precision`
    static int printedValueLength(double value, int precision);

    /// Return the number of characters needed to print the specified
    /// `value` with a single chatacter separator between every
    /// `groupSize` digits.
    static int printedValueLengthWithSeparator(bsls::Types::Int64 value,
                                               int                groupSize);

    /// Return the number of characters needed to print the specified
    /// `value` with the specified `precision` with a single character
    /// separator between every `groupSize` digits'.
    static int printedValueLengthWithSeparator(double value,
                                               int    precision,
                                               int    groupSize);

    /// Print the specified `value` to the specified `stream` with
    /// the specified `separator` character between every `groupSize`
    /// digits.
    static bsl::ostream& printValueWithSeparator(bsl::ostream&      stream,
                                                 bsls::Types::Int64 value,
                                                 int                groupSize,
                                                 char               separator);

    /// Print the specified `value` with the specified `precision` to the
    /// specified `stream` with the specified `separator` character between
    /// every `groupSize` digits.
    static bsl::ostream& printValueWithSeparator(bsl::ostream& stream,
                                                 double        value,
                                                 int           precision,
                                                 int           groupSize,
                                                 char          separator);

    /// Print the specified `str` to the specified `stream` centered in the
    /// `stream`s current `width`.  Return `stream`.
    static bsl::ostream& printStringCentered(bsl::ostream&            stream,
                                             const bslstl::StringRef& str);

    /// Print the specified `bytes` using the appropriate suffix (KB, MB,
    /// etc) to the specified `stream` with the specified `precision`.  The
    /// behavior is undefined unless `precision <= 3`.
    static bsl::ostream&
    printMemory(bsl::ostream& stream, bsls::Types::Int64 bytes, int precision);

    /// Return the length of the string printed when the specified `bytes`
    /// and `precision` are passed to `printMemory`.
    static int printedMemoryLength(bsls::Types::Int64 bytes, int precision);

    /// Print the specified `timeIntervalNs` which is assumed to be a
    /// number of nanoseconds to the specified `stream` with the specified
    /// `precision` in a readable form.
    static bsl::ostream& printTimeIntervalNs(bsl::ostream&      stream,
                                             bsls::Types::Int64 timeIntervalNs,
                                             int                precision);

    /// Return the length of the string printed when the specified
    /// `timeIntervalNs` and `precision` are passed to
    /// `printTimeIntervalNs`
    static int printedTimeIntervalNsLength(bsls::Types::Int64 timeIntervalNs,
                                           int                precision);

    /// Print the elapsed time of the specified `stopwatch` to the specified
    /// `stream` with the specified `precision` in a readable form.
    static bsl::ostream& printElapsedTime(bsl::ostream&          stream,
                                          const bsls::Stopwatch& stopwatch,
                                          int                    precision);

    /// Print the specified `num` with the appropriate ordinal suffix (st,
    /// nd, rd, th) to the specified `stream` and return the `stream`.
    static bsl::ostream& printOrdinal(bsl::ostream&      stream,
                                      bsls::Types::Int64 num);

    /// Return a printer for the specified `obj`.
    template <typename TYPE>
    static Printer<TYPE> printer(const TYPE& obj);

    /// Print the specified `stringRef` to the specified output `stream` at
    /// the (absolute value of) the optionally specified indentation
    /// `level` and return a reference to `stream`.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, the number of
    /// spaces per indentation level for this and all of its nested
    /// objects.  If `level` is negative, suppress indentation of the first
    /// line.  If `spacesPerLevel` is negative format the entire output on
    /// one line, suppressing all but the initial indentation (as governed
    /// by `level`).  If `stream` is not valid on entry, this operation has
    /// no effect.
    static bsl::ostream& stringRefPrint(bsl::ostream&            stream,
                                        const bslstl::StringRef& stringRef,
                                        int                      level = 0,
                                        int spacesPerLevel             = 4);
};

// =============
// class Printer
// =============

/// Printer for arbitrary types, with ouput operator specialized for some
/// types without an output operator
template <typename TYPE>
class Printer {
  private:
    // DATA
    const TYPE* d_obj_p;

  public:
    // CREATORS
    explicit Printer(const TYPE* obj)
    : d_obj_p(obj)
    {
    }

    // ACCESSORS
    const TYPE& obj() const;

    /// Print the `object` by using our own `operator<<`
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS
template <typename TYPE>
bsl::ostream& operator<<(bsl::ostream& stream, const Printer<TYPE>& p);

template <typename TYPE>
bsl::ostream& operator<<(bsl::ostream&                      stream,
                         const Printer<bsl::vector<TYPE> >& p);

template <typename TYPE1, typename TYPE2>
bsl::ostream& operator<<(bsl::ostream&                           stream,
                         const Printer<bsl::map<TYPE1, TYPE2> >& p);

template <typename TYPE>
bsl::ostream& operator<<(bsl::ostream&                   stream,
                         const Printer<bsl::set<TYPE> >& p);

template <typename TYPE1, typename TYPE2>
bsl::ostream& operator<<(bsl::ostream& stream,
                         const Printer<bsl::unordered_map<TYPE1, TYPE2> >& p);

template <typename TYPE>
bsl::ostream& operator<<(bsl::ostream&                             stream,
                         const Printer<bsl::unordered_set<TYPE> >& p);

template <typename TYPE1, typename TYPE2>
bsl::ostream& operator<<(bsl::ostream&                            stream,
                         const Printer<bsl::pair<TYPE1, TYPE2> >& p);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------
// class PrintUtil
// ---------------

template <typename TYPE>
inline Printer<TYPE> PrintUtil::printer(const TYPE& obj)
{
    return Printer<TYPE>(&obj);
}

// -------------
// class Printer
// -------------

// ACCESSORS
template <typename TYPE>
const TYPE& Printer<TYPE>::obj() const
{
    return *d_obj_p;
}

template <typename TYPE>
bsl::ostream&
Printer<TYPE>::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);

    stream << *this;

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

}  // close package namespace

// FREE OPERATORS
template <typename TYPE>
bsl::ostream& bmqst::operator<<(bsl::ostream&               stream,
                                const bmqst::Printer<TYPE>& p)
{
    return stream << p.obj();
}

template <typename TYPE>
bsl::ostream& bmqst::operator<<(bsl::ostream& stream,
                                const bmqst::Printer<bsl::vector<TYPE> >& p)
{
    stream << "[";

    if (!p.obj().empty()) {
        for (size_t i = 0; i < p.obj().size() - 1; ++i) {
            stream << bmqst::Printer<TYPE>(&p.obj()[i]) << ", ";
        }

        stream << bmqst::Printer<TYPE>(&p.obj().back());
    }

    return stream << "]";
}

template <typename TYPE1, typename TYPE2>
bsl::ostream&
bmqst::operator<<(bsl::ostream&                                  stream,
                  const bmqst::Printer<bsl::map<TYPE1, TYPE2> >& p)
{
    stream << "{";

    if (!p.obj().empty()) {
        typedef typename bsl::map<TYPE1, TYPE2>::const_iterator Iter;
        Iter iter     = p.obj().begin();
        Iter lastElem = --p.obj().end();
        for (; iter != lastElem; ++iter) {
            stream << bmqst::Printer<TYPE1>(&iter->first) << ":"
                   << bmqst::Printer<TYPE2>(&iter->second) << ", ";
        }

        stream << bmqst::Printer<TYPE1>(&iter->first) << ":"
               << bmqst::Printer<TYPE2>(&iter->second);
    }

    return stream << "}";
}

template <typename TYPE>
bsl::ostream& bmqst::operator<<(bsl::ostream&                          stream,
                                const bmqst::Printer<bsl::set<TYPE> >& p)
{
    stream << "{";

    if (!p.obj().empty()) {
        typedef typename bsl::set<TYPE>::const_iterator Iter;
        Iter                                            iter = p.obj().begin();
        Iter lastElem                                        = --p.obj().end();
        for (; iter != lastElem; ++iter) {
            stream << bmqst::Printer<TYPE>(&(*iter)) << ", ";
        }

        stream << bmqst::Printer<TYPE>(&(*iter));
    }

    return stream << "}";
}

template <typename TYPE>
bsl::ostream&
bmqst::operator<<(bsl::ostream&                                    stream,
                  const bmqst::Printer<bsl::unordered_set<TYPE> >& p)
{
    stream << "{";

    if (!p.obj().empty()) {
        typedef typename bsl::unordered_set<TYPE>::const_iterator Iter;
        Iter begin = p.obj().begin();
        Iter end   = p.obj().end();
        for (Iter iter = begin; iter != end; ++iter) {
            if (iter != begin) {
                stream << ", ";
            }
            stream << bmqst::Printer<TYPE>(&(*iter));
        }
    }

    return stream << "}";
}

template <typename TYPE1, typename TYPE2>
bsl::ostream&
bmqst::operator<<(bsl::ostream& stream,
                  const bmqst::Printer<bsl::unordered_map<TYPE1, TYPE2> >& p)
{
    stream << "{";

    typedef typename bsl::unordered_map<TYPE1, TYPE2>::const_iterator Iter;
    Iter begin = p.obj().begin();
    Iter end   = p.obj().end();
    for (Iter iter = begin; iter != end; ++iter) {
        if (iter != begin) {
            stream << ", ";
        }

        stream << bmqst::Printer<TYPE1>(&iter->first) << ":"
               << bmqst::Printer<TYPE2>(&iter->second);
    }

    return stream << "}";
}

template <typename TYPE1, typename TYPE2>
bsl::ostream&
bmqst::operator<<(bsl::ostream&                                   stream,
                  const bmqst::Printer<bsl::pair<TYPE1, TYPE2> >& p)
{
    return stream << '<' << p.obj().first << ", " << p.obj().second << '>';
}

}  // close enterprise namespace

#endif
