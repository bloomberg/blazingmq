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

// mwcu_printutil.h                                                   -*-C++-*-
#ifndef INCLUDED_MWCU_PRINTUTIL
#define INCLUDED_MWCU_PRINTUTIL

//@PURPOSE: Provide utilities for printing things.
//
//@NAMESPACES:
// mwcu::PrintUtil
//
//@FUNCTIONS:
//  indent:             write characters for indenting as per BDE 'indent'
//  newlineAndIndent:   write characters for indenting as per BDE
//                      'newlineAndIndent'
//  prettyBytes:        write a bytes amount using the best unit
//  prettyNumber:       write a number (Int64 or double) as groups of digits
//  prettyTimeInterval: write a time interval (in nanosecs) using the best unit
//
//@CLASSES:
// mwcu::Printer
//
//@DESCRIPTION:
// This component provides a set of utility functions and stream manipulators
// for writing numbers, time intervals, and bytes amounts in a human-readable
// format.
// The 'pretty*' functions exist in two styles:
//: o Procedural: The argument list consists of the destination output stream,
//:   followed by the value to write, possibly followed by formatting
//:   parameters (group size, group separator, etc). The function writes the
//:   value to the stream and returns void.
//: o Manipulator: The argument list consists of the value to write, possibly
//:   followed by formatting parameters (group size, group separator, etc).
//:   The function returns an object which, when inserted into an output
//:   stream, writes the value to the stream. This is similar to the standard
//:   library stream manipulators ('std::setw', 'std::setprecision', etc).
//
// The '*indent' functions exist only as manipulators that invoke the
// corresponding functions in the 'bdlb_print' component.
//
// All the functions provided by this component have sensible default values
// for the formatting parameters. Numbers are written following the US American
// convention (i.e. group digits by thousands, using the comma as separator).
// Bytes amounts and time intervals are written with two decimals.
//
/// Usage
///-----
// Write values using the procedural interface:
//..
//  // os is a bsl::ostream                  output:
//  prettyNumber(os, 1234567, 2, '.'); // 1.23.45.67
//  prettyNumber(os, 1234567);         // 1,234,567
//  prettyBytes(os, 1025);             // 1.00 KB
//  prettyTimeInterval(os, 12432);     // 12.43 us
//..
//
// Write the same values using manipulators:
//..
//  os << prettyNumber(1234567, 2, '.'); // 1.23.45.67
//  os << prettyNumber(1234567);         // 1,234,567
//  os << prettyBytes(1025);             // 1.00 KB
//  os << prettyTimeInterval(12432);     // 12.43 us
//..

// MWC

// BDE
#include <bdlb_print.h>
#include <bsl_map.h>
#include <bsl_ostream.h>
#include <bsl_set.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwcu {

// FORWARD DECLARATIONS
template <typename TYPE>
class Printer;

// ===================
// namespace PrintUtil
// ===================

namespace PrintUtil {

// FORWARD DECLARATIONS
struct IndentManipulator;
struct NewlineAndIndentManipulator;
struct PrettyBytesManipulator;
struct PrettyDoubleManipulator;
struct PrettyIntManipulator;
struct PrettyTimeIntervalManipulator;

// FREE FUNCTIONS

/// Print the specified `value` to the specified `stream` with the specified
/// `separator` character between every `groupSize` digits.
bsl::ostream& prettyNumber(bsl::ostream& stream,
                           int           value,
                           int           groupSize = 3,
                           char          separator = ',');
bsl::ostream& prettyNumber(bsl::ostream&      stream,
                           bsls::Types::Int64 value,
                           int                groupSize = 3,
                           char               separator = ',');

/// Return a stream manipulator that inserts the specified `value` with the
/// specified `separator` character between every `groupSize` digits.
PrettyIntManipulator
prettyNumber(int value, int groupSize = 3, char separator = ',');
PrettyIntManipulator prettyNumber(bsls::Types::Int64 value,
                                  int                groupSize = 3,
                                  char               separator = ',');

/// Value is truncated, not rounded...
bsl::ostream& prettyNumber(bsl::ostream& stream,
                           double        value,
                           int           precision = 2,
                           int           groupSize = 3,
                           char          separator = ',');

/// Return a stream manipulator that inserts the specified `value` with the
/// specified `precision` with the specified `separator` character between
/// every `groupSize` digits. `value` is truncated, not rounded.
PrettyDoubleManipulator prettyNumber(double value,
                                     int    precision = 2,
                                     int    groupSize = 3,
                                     char   separator = ',');

/// Print the specified `bytes` using the appropriate suffix (KB, MB, etc)
/// to the specified `stream` with the specified `precision`. The behavior
/// is undefined unless `precision <= 3`.
bsl::ostream&
prettyBytes(bsl::ostream& stream, bsls::Types::Int64 bytes, int precision = 2);

/// Return a stream manipulator that inserts the specified `bytes` using the
/// appropriate suffix (KB, MB, etc) with the specified `precision`. The
/// behavior is undefined unless `precision <= 3`.
PrettyBytesManipulator prettyBytes(bsls::Types::Int64 bytes,
                                   int                precision = 2);

/// Print the specified `timeNs` which is assumed to be a number of
/// nanoseconds to the specified `stream` with the specified `precision` in
/// a readable form. Values are rounded to the nearest.
bsl::ostream& prettyTimeInterval(bsl::ostream&      stream,
                                 bsls::Types::Int64 timeNs,
                                 int                precision = 2);

/// Return a stream manipulator that inserts the specified `timeNs` which is
/// assumed to be a number of nanoseconds with the specified `precision` in
/// a readable form. Values are rounded to the nearest.
PrettyTimeIntervalManipulator prettyTimeInterval(bsls::Types::Int64 timeNs,
                                                 int precision = 2);

/// Return a stream manipulator that emits the number of spaces (` `) equal
/// to the absolute value of the product of the specified `level` and
/// `spacesPerLevel` or, if `level` is negative, nothing at all.
IndentManipulator indent(int level, int spacesPerLevel);

/// Return a stream manipulator that emits a newline ('\n') followed by the
/// number of spaces (` `) equal to the absolute value of the product of the
/// specified `level` and `spacesPerLevel` or, if `spacesPerLevel` is
/// negative, emit a single space (and *no* newline).
NewlineAndIndentManipulator newlineAndIndent(int level, int spacesPerLevel);

/// Return a printer for the specified `obj`.
template <typename TYPE>
Printer<TYPE> printer(const TYPE& obj);

/// Print the specified `manipulator` into the specified output `stream` and
/// return `stream`.
bsl::ostream& operator<<(bsl::ostream&               stream,
                         const PrettyIntManipulator& manipulator);
bsl::ostream& operator<<(bsl::ostream&                  stream,
                         const PrettyDoubleManipulator& manipulator);
bsl::ostream& operator<<(bsl::ostream&                 stream,
                         const PrettyBytesManipulator& manipulator);
bsl::ostream& operator<<(bsl::ostream&                        stream,
                         const PrettyTimeIntervalManipulator& manipulator);
bsl::ostream& operator<<(bsl::ostream&            stream,
                         const IndentManipulator& manipulator);
bsl::ostream& operator<<(bsl::ostream&                      stream,
                         const NewlineAndIndentManipulator& manipulator);

// ===========================
// struct PrettyIntManipulator
// ===========================

struct PrettyIntManipulator {
    // DATA
    bsls::Types::Int64 d_value;
    int                d_groupSize;
    char               d_separator;

    // CREATORS

    /// Create a `PrettyIntManipulator` object.
    PrettyIntManipulator(bsls::Types::Int64 value,
                         int                groupSize,
                         char               separator);

    // ACCESSORS

    /// Print to the specified output `stream`, using the optionally
    /// specified indentation `level` and the optionally specified
    /// `spacesPerLevel` the value of this object, as per the contract of
    /// the standard BDE print method. If `stream` is not valid on entry,
    /// this function has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// ==============================
// struct PrettyDoubleManipulator
// ==============================

struct PrettyDoubleManipulator {
    // DATA
    double d_value;
    int    d_precision;
    int    d_groupSize;
    char   d_separator;

    // CREATORS

    /// Create a `PrettyDoubleManipulator` object.
    PrettyDoubleManipulator(double value,
                            int    precision,
                            int    groupSize,
                            char   separator);

    // ACCESSORS

    /// Print to the specified output `stream`, using the optionally
    /// specified indentation `level` and the optionally specified
    /// `spacesPerLevel` the value of this object, as per the contract of
    /// the standard BDE print method. If `stream` is not valid on entry,
    /// this function has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// =============================
// struct PrettyBytesManipulator
// =============================

struct PrettyBytesManipulator {
    // DATA
    bsls::Types::Int64 d_bytes;
    int                d_precision;

    // CREATORS

    /// Create a `PrettyBytesManipulator` object.
    PrettyBytesManipulator(bsls::Types::Int64 bytes, int precision);

    // ACCESSORS

    /// Print to the specified output `stream`, using the optionally
    /// specified indentation `level` and the optionally specified
    /// `spacesPerLevel` the value of this object, as per the contract of
    /// the standard BDE print method. If `stream` is not valid on entry,
    /// this function has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// ====================================
// struct PrettyTimeIntervalManipulator
// ====================================

struct PrettyTimeIntervalManipulator {
    // DATA
    bsls::Types::Int64 d_timeNs;
    int                d_precision;

    // CREATORS

    /// Create a `PrettyTimeIntervalManipulator` object.
    PrettyTimeIntervalManipulator(bsls::Types::Int64 timeNs, int precision);

    // ACCESSORS

    /// Print to the specified output `stream`, using the optionally
    /// specified indentation `level` and the optionally specified
    /// `spacesPerLevel` the value of this object, as per the contract of
    /// the standard BDE print method. If `stream` is not valid on entry,
    /// this function has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// ========================
// struct IndentManipulator
// ========================

struct IndentManipulator {
    // DATA
    int d_level;
    int d_spacesPerLevel;

    // CREATORS

    /// Create a `IndentManipulator` object.
    IndentManipulator(int level, int spacesPerLevel);
};

// ==================================
// struct NewlineAndIndentManipulator
// ==================================

struct NewlineAndIndentManipulator {
    // DATA
    int d_level;
    int d_spacesPerLevel;

    // CREATORS

    /// Create a `NewlineAndIndentManipulator` object.
    NewlineAndIndentManipulator(int level, int spacesPerLevel);
};

}  // close namespace PrintUtil

// =============
// class Printer
// =============

/// Printer for arbitrary types, with output operator specialized for some
/// types without an output operator
template <typename TYPE>
class Printer {
  private:
    // PRIVATE DATA
    const TYPE* d_obj_p;

  public:
    // CREATORS

    /// Create a `Printer` object.
    explicit Printer(const TYPE* obj);

  public:
    // ACCESSORS
    const TYPE& obj() const;

    /// Print the `object` by using our own `operator<<`
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS
template <typename TYPE>
bsl::ostream& operator<<(bsl::ostream& stream, const Printer<TYPE>& printer);
template <typename TYPE>
bsl::ostream& operator<<(bsl::ostream&                      stream,
                         const Printer<bsl::vector<TYPE> >& printer);

template <typename TYPE1, typename TYPE2, typename TYPE3>
bsl::ostream&
operator<<(bsl::ostream&                                  stream,
           const Printer<bsl::map<TYPE1, TYPE2, TYPE3> >& printer);
template <typename TYPE1, typename TYPE2>
bsl::ostream& operator<<(bsl::ostream&                           stream,
                         const Printer<bsl::set<TYPE1, TYPE2> >& printer);
template <typename TYPE1, typename TYPE2, typename TYPE3, typename TYPE4>
bsl::ostream& operator<<(
    bsl::ostream&                                                   stream,
    const Printer<bsl::unordered_map<TYPE1, TYPE2, TYPE3, TYPE4> >& printer);
template <typename TYPE1, typename TYPE2, typename TYPE3>
bsl::ostream&
operator<<(bsl::ostream&                                            stream,
           const Printer<bsl::unordered_set<TYPE1, TYPE2, TYPE3> >& printer);
template <typename TYPE1, typename TYPE2>
bsl::ostream& operator<<(bsl::ostream&                            stream,
                         const Printer<bsl::pair<TYPE1, TYPE2> >& printer);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------
// namespace PrintUtil
// -------------------

inline PrintUtil::PrettyIntManipulator
PrintUtil::prettyNumber(int value, int groupSize, char separator)
{
    return PrintUtil::PrettyIntManipulator(
        static_cast<bsls::Types::Int64>(value),
        groupSize,
        separator);
}

inline PrintUtil::PrettyIntManipulator
PrintUtil::prettyNumber(bsls::Types::Int64 value,
                        int                groupSize,
                        char               separator)
{
    return PrintUtil::PrettyIntManipulator(value, groupSize, separator);
}

inline PrintUtil::PrettyDoubleManipulator
PrintUtil::prettyNumber(double value,
                        int    precision,
                        int    groupSize,
                        char   separator)
{
    return PrintUtil::PrettyDoubleManipulator(value,
                                              precision,
                                              groupSize,
                                              separator);
}

inline PrintUtil::PrettyBytesManipulator
PrintUtil::prettyBytes(bsls::Types::Int64 bytes, int precision)
{
    return PrintUtil::PrettyBytesManipulator(bytes, precision);
}

inline PrintUtil::PrettyTimeIntervalManipulator
PrintUtil::prettyTimeInterval(bsls::Types::Int64 timeNs, int precision)
{
    return PrintUtil::PrettyTimeIntervalManipulator(timeNs, precision);
}

inline PrintUtil::IndentManipulator PrintUtil::indent(int level,
                                                      int spacesPerLevel)
{
    return PrintUtil::IndentManipulator(level, spacesPerLevel);
}

inline PrintUtil::NewlineAndIndentManipulator
PrintUtil::newlineAndIndent(int level, int spacesPerLevel)
{
    return PrintUtil::NewlineAndIndentManipulator(level, spacesPerLevel);
}

template <typename TYPE>
inline Printer<TYPE> PrintUtil::printer(const TYPE& obj)
{
    return Printer<TYPE>(&obj);
}

inline bsl::ostream&
PrintUtil::operator<<(bsl::ostream&                          stream,
                      const PrintUtil::PrettyIntManipulator& manipulator)
{
    PrintUtil::prettyNumber(stream,
                            manipulator.d_value,
                            manipulator.d_groupSize,
                            manipulator.d_separator);
    return stream;
}

inline bsl::ostream&
PrintUtil::operator<<(bsl::ostream&                  stream,
                      const PrettyDoubleManipulator& manipulator)
{
    PrintUtil::prettyNumber(stream,
                            manipulator.d_value,
                            manipulator.d_precision,
                            manipulator.d_groupSize,
                            manipulator.d_separator);
    return stream;
}

inline bsl::ostream&
PrintUtil::operator<<(bsl::ostream&                 stream,
                      const PrettyBytesManipulator& manipulator)
{
    PrintUtil::prettyBytes(stream,
                           manipulator.d_bytes,
                           manipulator.d_precision);
    return stream;
}

inline bsl::ostream&
PrintUtil::operator<<(bsl::ostream&                        stream,
                      const PrettyTimeIntervalManipulator& manipulator)
{
    PrintUtil::prettyTimeInterval(stream,
                                  manipulator.d_timeNs,
                                  manipulator.d_precision);
    return stream;
}

inline bsl::ostream&
PrintUtil::operator<<(bsl::ostream&            stream,
                      const IndentManipulator& manipulator)
{
    bdlb::Print::indent(stream,
                        manipulator.d_level,
                        manipulator.d_spacesPerLevel);
    return stream;
}

inline bsl::ostream&
PrintUtil::operator<<(bsl::ostream&                      stream,
                      const NewlineAndIndentManipulator& manipulator)
{
    bdlb::Print::newlineAndIndent(stream,
                                  manipulator.d_level,
                                  manipulator.d_spacesPerLevel);
    return stream;
}

// --------------------------------------
// struct PrintUtil::PrettyIntManipulator
// --------------------------------------

// CREATORS
inline PrintUtil::PrettyIntManipulator::PrettyIntManipulator(
    bsls::Types::Int64 value,
    int                groupSize,
    char               separator)
: d_value(value)
, d_groupSize(groupSize)
, d_separator(separator)
{
    // NOTHING
}

// -----------------------------------------
// struct PrintUtil::PrettyDoubleManipulator
// -----------------------------------------

// CREATORS
inline PrintUtil::PrettyDoubleManipulator::PrettyDoubleManipulator(
    double value,
    int    precision,
    int    groupSize,
    char   separator)
: d_value(value)
, d_precision(precision)
, d_groupSize(groupSize)
, d_separator(separator)
{
    // NOTHING
}

// ACCESSORS
inline bsl::ostream&
PrintUtil::PrettyDoubleManipulator::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
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

// ----------------------------------------
// struct PrintUtil::PrettyBytesManipulator
// ----------------------------------------

// CREATORS
inline PrintUtil::PrettyBytesManipulator::PrettyBytesManipulator(
    bsls::Types::Int64 bytes,
    int                precision)
: d_bytes(bytes)
, d_precision(precision)
{
    // NOTHING
}

// ACCESSORS
inline bsl::ostream&
PrintUtil::PrettyBytesManipulator::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
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

// -----------------------------------------------
// struct PrintUtil::PrettyTimeIntervalManipulator
// -----------------------------------------------

// CREATORS
inline PrintUtil::PrettyTimeIntervalManipulator::PrettyTimeIntervalManipulator(
    bsls::Types::Int64 timeNs,
    int                precision)
: d_timeNs(timeNs)
, d_precision(precision)
{
    // NOTHING
}

// ACCESSORS
inline bsl::ostream&
PrintUtil::PrettyTimeIntervalManipulator::print(bsl::ostream& stream,
                                                int           level,
                                                int spacesPerLevel) const
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

// -----------------------------------
// struct PrintUtil::IndentManipulator
// -----------------------------------

// CREATORS
inline PrintUtil::IndentManipulator::IndentManipulator(int level,
                                                       int spacesPerLevel)
: d_level(level)
, d_spacesPerLevel(spacesPerLevel)
{
    // NOTHING
}

// ---------------------------------------------
// struct PrintUtil::NewlineAndIndentManipulator
// ---------------------------------------------

// CREATORS
inline PrintUtil::NewlineAndIndentManipulator::NewlineAndIndentManipulator(
    int level,
    int spacesPerLevel)
: d_level(level)
, d_spacesPerLevel(spacesPerLevel)
{
    // NOTHING
}

// -------------
// class Printer
// -------------

// CREATORS
template <typename TYPE>
inline Printer<TYPE>::Printer(const TYPE* obj)
: d_obj_p(obj)
{
    // NOTHING
}

// ACCESSORS
template <typename TYPE>
inline const TYPE& Printer<TYPE>::obj() const
{
    return *d_obj_p;
}

template <typename TYPE>
inline bsl::ostream&
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
inline bsl::ostream& mwcu::operator<<(bsl::ostream&        stream,
                                      const Printer<TYPE>& printer)
{
    return stream << printer.obj();
}

template <typename TYPE>
inline bsl::ostream&
mwcu::operator<<(bsl::ostream&                      stream,
                 const Printer<bsl::vector<TYPE> >& printer)
{
    stream << "[";

    if (!printer.obj().empty()) {
        for (size_t i = 0; i < printer.obj().size() - 1; ++i) {
            stream << Printer<TYPE>(&printer.obj()[i]) << ", ";
        }

        stream << Printer<TYPE>(&printer.obj().back());
    }

    stream << "]";

    return stream;
}

template <typename TYPE1, typename TYPE2, typename TYPE3>
inline bsl::ostream&
mwcu::operator<<(bsl::ostream&                                  stream,
                 const Printer<bsl::map<TYPE1, TYPE2, TYPE3> >& printer)
{
    stream << "{";

    if (!printer.obj().empty()) {
        typedef typename bsl::map<TYPE1, TYPE2, TYPE3>::const_iterator Iter;
        Iter iter     = printer.obj().begin();
        Iter lastElem = --printer.obj().end();
        for (; iter != lastElem; ++iter) {
            stream << Printer<TYPE1>(&iter->first) << ":"
                   << Printer<TYPE2>(&iter->second) << ", ";
        }

        stream << Printer<TYPE1>(&iter->first) << ":"
               << Printer<TYPE2>(&iter->second);
    }

    stream << "}";

    return stream;
}

template <typename TYPE1, typename TYPE2>
inline bsl::ostream&
mwcu::operator<<(bsl::ostream&                           stream,
                 const Printer<bsl::set<TYPE1, TYPE2> >& printer)
{
    stream << "{";

    if (!printer.obj().empty()) {
        typedef typename bsl::set<TYPE1, TYPE2>::const_iterator Iter;
        Iter iter     = printer.obj().begin();
        Iter lastElem = --printer.obj().end();
        for (; iter != lastElem; ++iter) {
            stream << Printer<TYPE1>(&(*iter)) << ", ";
        }

        stream << Printer<TYPE1>(&(*iter));
    }

    stream << "}";

    return stream;
}

template <typename TYPE1, typename TYPE2, typename TYPE3>
inline bsl::ostream& mwcu::operator<<(
    bsl::ostream&                                            stream,
    const Printer<bsl::unordered_set<TYPE1, TYPE2, TYPE3> >& printer)
{
    stream << "{";

    if (!printer.obj().empty()) {
        typedef
            typename bsl::unordered_set<TYPE1, TYPE2, TYPE3>::const_iterator
                Iter;
        Iter    begin = printer.obj().begin();
        Iter    end   = printer.obj().end();
        for (Iter iter = begin; iter != end; ++iter) {
            if (iter != begin) {
                stream << ", ";
            }
            stream << Printer<TYPE1>(&(*iter));
        }
    }

    stream << "}";

    return stream;
}

template <typename TYPE1, typename TYPE2, typename TYPE3, typename TYPE4>
inline bsl::ostream& mwcu::operator<<(
    bsl::ostream&                                                   stream,
    const Printer<bsl::unordered_map<TYPE1, TYPE2, TYPE3, TYPE4> >& printer)
{
    stream << "{";

    typedef
        typename bsl::unordered_map<TYPE1, TYPE2, TYPE3, TYPE4>::const_iterator
            Iter;
    Iter    begin = printer.obj().begin();
    Iter    end   = printer.obj().end();
    for (Iter iter = begin; iter != end; ++iter) {
        if (iter != begin) {
            stream << ", ";
        }

        stream << Printer<TYPE1>(&iter->first) << ":"
               << Printer<TYPE2>(&iter->second);
    }

    stream << "}";

    return stream;
}

template <typename TYPE1, typename TYPE2>
inline bsl::ostream&
mwcu::operator<<(bsl::ostream&                            stream,
                 const Printer<bsl::pair<TYPE1, TYPE2> >& printer)
{
    return stream << '<' << printer.obj().first << ", " << printer.obj().second
                  << '>';
}

}  // close enterprise namespace

#endif
