// Copyright 2022-2024 Bloomberg Finance L.P.
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

// bmqu_jsonprinter.h                                                 -*-C++-*-
#ifndef INCLUDED_BMQU_JSONPRINTER
#define INCLUDED_BMQU_JSONPRINTER

//@PURPOSE: Provide a mechanism to print key-value pairs in JSON format.
//
//@CLASSES:
//  bmqu::JsonPrinter:  Mechanism to print key-value pairs in JSON format.
//
//@DESCRIPTION: 'bmqu::JsonPrinter' provides a mechanism to print key-value
// pairs in JSON format.
//
/// Usage
///-----
// First, specify field names for printer:
//..
//  bsl::vector<const char*> fields;
//  fields.push_back("Queue URI");
//  fields.push_back("QueueKey");
//  fields.push_back("Number of AppIds");
//..
//
// Next, create an instance of bmqu::AlignedPrinter:
//..
//  bsl::stringstream             output;
//  bmqu::JsonPrinter<true, 0, 4> printer(output, &fields);
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
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslmf_enableif.h>
#include <bsls_assert.h>
#include <bslstl_stringref.h>

namespace BloombergLP {
namespace bmqu {

template <typename T>
bool addQuotes(const T&)
{
    return true;
}

template <>
inline bool addQuotes<bslstl::StringRef>(const bslstl::StringRef& value)
{
    bsl::size_t pos = value.find_first_not_of(" \n");
    return (pos == bsl::string::npos || value[pos] != '{');
}

// =================
// class JsonPrinter
// =================

/// Mechanism to print key-value pairs in JSON format.
template <bool pretty      = false,
          bool braceNeeded = true,
          int  braceIndent = 0,
          int  fieldIndent = 4>
class JsonPrinter {
  private:
    // DATA
    bsl::ostream&                   d_ostream;
    const bsl::vector<const char*>* d_fields_p;
    unsigned int                    d_counter;

    // NOT IMPLEMENTED
    JsonPrinter(const JsonPrinter&);
    JsonPrinter& operator=(const JsonPrinter&);

  public:
    // CREATORS

    /// Create an instance that will print to the specified `stream` a JSON
    /// object with the specified `fields` with the optionally specified
    /// `indent`.  Behavior is undefined unless `indent` >= 0 and at least one
    /// field is present in the `fields`.
    JsonPrinter(bsl::ostream& stream, const bsl::vector<const char*>* fields);

    ~JsonPrinter();

    // MANIPULATORS

    /// Print the specified `value` to the stream held by this printer
    /// instance.  Behavior is undefined unless there exists a field
    /// corresponding to the `value`.
    template <typename TYPE>
    JsonPrinter& operator<<(const TYPE& value);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------
// JsonPrinter
// -----------

template <bool pretty, bool braceNeeded, int braceIndent, int fieldIndent>
inline JsonPrinter<pretty, braceNeeded, braceIndent, fieldIndent>::JsonPrinter(
    bsl::ostream&                   stream,
    const bsl::vector<const char*>* fields)
: d_ostream(stream)
, d_fields_p(fields)
, d_counter(0)
{
    BSLS_ASSERT_SAFE(0 < d_fields_p->size());
    if (braceNeeded) {
        if (braceIndent > 0) {
            d_ostream << bsl::setw(braceIndent) << ' ';
        }
        d_ostream << '{';
        if (pretty) {
            d_ostream << '\n';
        }
    }
}

template <bool pretty, bool braceNeeded, int braceIndent, int fieldIndent>
inline JsonPrinter<pretty, braceNeeded, braceIndent, fieldIndent>::
    ~JsonPrinter()
{
    if (braceNeeded) {
        if (pretty) {
            d_ostream << '\n';
        }
        if (pretty && braceIndent > 0) {
            d_ostream << bsl::setw(braceIndent) << ' ';
        }
        d_ostream << "}";
    }
}

template <bool pretty, bool braceNeeded, int braceIndent, int fieldIndent>
template <typename TYPE>
inline JsonPrinter<pretty, braceNeeded, braceIndent, fieldIndent>&
JsonPrinter<pretty, braceNeeded, braceIndent, fieldIndent>::operator<<(
    const TYPE& value)
{
    BSLS_ASSERT_SAFE(d_counter < d_fields_p->size());

    if (d_counter != 0) {
        d_ostream << ',' << (pretty ? '\n' : ' ');
    }

    if (pretty) {
        d_ostream << bsl::setw(fieldIndent) << ' ';
    }

    const bool quotes = addQuotes(value);
    d_ostream << '\"' << (*d_fields_p)[d_counter] << "\": ";
    if (quotes) {
        d_ostream << '\"';
    }
    d_ostream << value;
    if (quotes) {
        d_ostream << '\"';
    }

    ++d_counter;

    return *this;
}

}  // close package namespace
}  // close enterprise namespace

#endif
