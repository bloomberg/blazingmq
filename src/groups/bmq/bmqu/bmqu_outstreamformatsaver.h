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

// bmqu_outstreamformatsaver.h                                        -*-C++-*-
#ifndef INCLUDED_BMQU_OUTSTREAMFORMATSAVER
#define INCLUDED_BMQU_OUTSTREAMFORMATSAVER

//@PURPOSE: provide a scoped guard for an ostream's format state.
//
//@CLASSES:
//  OutStreamFormatSaver: a scoped guard for an ostream's format state
//
//@DESCRIPTION: This component defines a mechanism, 'OutStreamFormatSaver',
// implementing a scoped guard for an ostream's format state.
//
/// Usage
///-----
// This section illustrates intended usage of this component.
//
/// Example 1: Manipulating 'bsl::cout' inside a function
///  - - - - - - - - - - - - - - - - - - - - - - - - - -
// Suppose we have a function 'printAsHex' that accepts a stream and an integer
// and prints the integer as hexadecimal with formatting as follows:
//..
//  void printAsHex(bsl::ostream& out, int myInt)
//  {
//      OutStreamFormatSaver fmtSaver(bsl::cout);
//
//      bsl::cout << bsl::showbase
//                << bsl::hex
//                << bsl::setfill('0')
//                << bsl::internal
//                << bsl::setw(10);
//
//     bsl::cout << myInt << '\n';
//  }
//..
// First, we call the function with 'bsl::cout' and an integer.
//..
//  const int myInt = 16263860U;
//
//  printAsHex(bsl::cout, myInt); // Outputs "0x00f82ab4"
//..
// Finally, we print the integer to 'bsl::cout'.
//..
//  bsl::cout << myInt << '\n';   // Outputs "16263860"
//                                // Output without 'fmtSaver': "0xf82ab4"
//..
//
/// Example 2: Restoring the state of 'cout' after manipulating it
///  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// First, we create a 'OutStreamFormatSaver' and pass to it 'bsl::cout'.
//..
//  OutStreamFormatSaver fmtSaver(bsl::cout);
//..
// Then, we manipulate 'bsl::cout' and print an integer as hexadecimal with
// formatting as follows:
//..
//  const int myInt = 16263860U;
//
//  // Set formatting
//  bsl::cout << bsl::showbase
//            << bsl::hex
//            << bsl::setfill('0')
//            << bsl::internal
//            << bsl::setw(10);
//
//  bsl::cout << myInt << '\n';    // Outputs "0x00f82ab4"
//..
// Next, we restore the state of 'bsl::cout' using the format saver's
// 'restore' method.
//..
//  fmtSaver.restore();
//
//  bsl::cout << myInt << '\n';   // Outputs "16263860"
//                                // Output w/o call to 'restore': "0xf82ab4"
//..
// Then, we again manipulate 'bsl::cout' and print a float with formatting as
// follows:
//..
//  const float myFloat = 12345.1;
//
//  // Set formatting
//  bsl::cout << bsl::setprecision(3)
//            << bsl::scientific
//            << bsl::showpos;
//
//  bsl::cout << myFloat << '\n'; // Outputs "+1.235e+04"
//..
// Finally, we again restore the state of 'bsl::cout' using the format saver's
// 'restore' method.
//..
//  fmtSaver.restore();
//
//  bsl::cout << myFloat << '\n'; // Outputs "12345.1"
//                                // Output w/o call to 'restore': "+1.235e+04"
//..

// BDE
#include <bdlb_nullablevalue.h>
#include <bsl_iostream.h>

namespace BloombergLP {
namespace bmqu {

// ====================
// OutStreamFormatSaver
// ====================

/// This mechanism implements a scoped guard for an ostream's format state.
class OutStreamFormatSaver {
  private:
    // DATA
    bdlb::NullableValue<bsl::ostream> d_oldState;
    // ostream state saved by this object

    bsl::ostream& d_out;
    // ostream proctored by this object

  private:
    // NOT IMPLEMENTED
    OutStreamFormatSaver(const OutStreamFormatSaver&);             // = delete
    OutStreamFormatSaver& operator=(const OutStreamFormatSaver&);  // = delete

  public:
    // CREATORS

    /// Create a proctor object that saves the format state of the specified
    /// `out`.
    explicit OutStreamFormatSaver(bsl::ostream& out);

    /// Destroy this proctor object and restore the format state of the
    /// ostream provided at construction to what it was at that time, unless
    /// `reset()` was previously invoked in which case this destructor is a
    /// no-op.
    ~OutStreamFormatSaver();

    // MANIPULATORS

    /// Restore the format state of the ostream provided at construction to
    /// what it was at that time, unless `reset()` was previously invoked in
    /// which case `restore` is a no-op.
    void restore();

    /// Reset the saved state.
    void reset();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class OutStreamFormatSaver
// --------------------------

// CREATORS
inline OutStreamFormatSaver::OutStreamFormatSaver(bsl::ostream& out)
: d_out(out)
{
    d_oldState.makeValueInplace(static_cast<bsl::streambuf*>(0)).copyfmt(out);
}

// MANIPULATORS
inline void OutStreamFormatSaver::restore()
{
    if (!d_oldState.isNull()) {
        d_out.copyfmt(d_oldState.value());
    }
}

inline void OutStreamFormatSaver::reset()
{
    d_oldState.reset();
}

}  // close package namespace
}  // close enterprise namespace

#endif
