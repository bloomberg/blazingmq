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

// bmqu_alignedprinter.h                                              -*-C++-*-
#ifndef INCLUDED_BMQU_ALIGNEDPRINTER
#define INCLUDED_BMQU_ALIGNEDPRINTER

//@PURPOSE: Provide a mechanism to print key-value pairs aligned.
//
//@CLASSES:
//  bmqu::AlignedPrinter:  Mechanism to print key-value pairs aligned.
//
//@DESCRIPTION: 'bmqu::AlignedPrinter' provides a mechanism to print key-value
// pairs in an aligned manner.
//
/// Usage
///-----
// First, specify field names for printer:
//..
//  bsl::vector<bsl::string> fields;
//  fields.emplace_back("Queue URI");
//  fields.emplace_back("QueueKey");
//  fields.emplace_back("Number of AppIds");
//..
//
// Next, create an instance of bmqu::AlignedPrinter:
//..
//  bsl::stringstream    output;
//  const int            indent = 8;
//  bmqu::AlignedPrinter printer(output, &fields, indent);
//..
//
// Last, print field values accordingly:
//..
//  bsl::string uri = "bmq://bmq.tutorial.workqueue/sample-queue";
//  bsl::string queueKey = "sample";
//  const int   num = 1;
//  printer << uri << queueKey << num;
//..
//

// BDE
#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqu {

// ====================
// class AlignedPrinter
// ====================

/// Mechanism to print key-value pairs in an aligned manner.
class AlignedPrinter {
  private:
    // DATA
    bsl::ostream&                   d_ostream;
    const bsl::vector<bsl::string>* d_fields_p;
    int                             d_indent;
    int                             d_width;
    unsigned int                    d_counter;

  private:
    // NOT IMPLEMENTED
    AlignedPrinter(const AlignedPrinter&);
    AlignedPrinter& operator=(const AlignedPrinter&);

  public:
    // CREATORS

    /// Create an instance that will print to the specified `stream` the
    /// specified `fields` with the optionally specified `indent`.  Behavior
    /// is undefined unless `indent` >= 0 and at least one field is present
    /// in the `fields`.
    AlignedPrinter(bsl::ostream&                   stream,
                   const bsl::vector<bsl::string>* fields,
                   int                             indent = 4);

    // MANIPULATORS

    /// Print the specified `value` to the stream held by this printer
    /// instance.  Behavior is undefined unless there exists a field
    /// corresponding to the `value`.
    template <typename TYPE>
    AlignedPrinter& operator<<(const TYPE& value);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------
// AlignedPrinter
// --------------

inline AlignedPrinter::AlignedPrinter(bsl::ostream&                   stream,
                                      const bsl::vector<bsl::string>* fields,
                                      int                             indent)
: d_ostream(stream)
, d_fields_p(fields)
, d_indent(indent)
, d_width(0)
, d_counter(0)
{
    BSLS_ASSERT_SAFE(0 <= d_indent);
    BSLS_ASSERT_SAFE(0 < d_fields_p->size());

    int maxLen = static_cast<int>((*d_fields_p)[0].length());
    for (unsigned int i = 1; i < d_fields_p->size(); ++i) {
        int len = static_cast<int>((*d_fields_p)[i].length());
        if (maxLen < len) {
            maxLen = len;
        }
    }

    d_width = maxLen + 4;  // 4 spaces b/w longest field and ':' character
}

template <typename TYPE>
inline AlignedPrinter& AlignedPrinter::operator<<(const TYPE& value)
{
    BSLS_ASSERT_SAFE(d_counter < d_fields_p->size());

    d_ostream << bsl::setw(d_indent) << ' ' << (*d_fields_p)[d_counter]
              << bsl::setw(static_cast<int>(d_width -
                                            (*d_fields_p)[d_counter].length()))
              << ": " << value << '\n';

    ++d_counter;
    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
