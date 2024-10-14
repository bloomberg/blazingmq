// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqio_listenoptions.h                                              -*-C++-*-
#ifndef INCLUDED_BMQIO_LISTENOPTIONS
#define INCLUDED_BMQIO_LISTENOPTIONS

//@PURPOSE: Provide a type encapsulating options for 'ChannelFactory::listen'.
//
//@CLASSES:
//  bmqio::ListenOptions: type encapsulating 'ChannelFactory::listen' options
//
//@DESCRIPTION: This component defines a value-semantic type,
// 'bmqio::ListenOptions', which encapsulates various listen options
// passed to 'bmqio::ChannelFactory::listen'.

#include <bmqvt_propertybag.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_limits.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

namespace bmqio {

// ===================
// class ListenOptions
// ===================

/// Listen options used when calling `bmqio::ChannelFactory::listen`.
class ListenOptions {
  private:
    // DATA
    bsl::string d_endpoint;  // implementation-defined endpoint
                             // to listen on

    bmqvt::PropertyBag d_properties;  // additional properties

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ListenOptions, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a default `ListenOptions` object with with unlimited
    /// attempts and a 10 second interval between attempts.
    explicit ListenOptions(bslma::Allocator* basicAllocator = 0);

    ListenOptions(const ListenOptions& original,
                  bslma::Allocator*    basicAllocator = 0);

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ListenOptions& operator=(const ListenOptions& rhs);

    /// Set the `endpoint` property of this object and return a reference
    /// providing modifiable access to this object.
    ListenOptions& setEndpoint(const bslstl::StringRef& value);

    /// Return a reference providing modifiable access to the `properties`
    /// property of this object.
    bmqvt::PropertyBag& properties();

    // ACCESSORS

    /// Return a reference providing const access to the `endpoint` property
    /// of this object.
    const bsl::string& endpoint() const;

    /// Return a reference providing const access to the `properties`
    /// property of this object.
    const bmqvt::PropertyBag& properties() const;

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

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const ListenOptions& obj);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// class ListenOptions
// --------------------

// CREATORS
inline ListenOptions::ListenOptions(bslma::Allocator* basicAllocator)
: d_endpoint(basicAllocator)
, d_properties(basicAllocator)
{
}

inline ListenOptions::ListenOptions(const ListenOptions& original,
                                    bslma::Allocator*    basicAllocator)
: d_endpoint(original.d_endpoint, basicAllocator)
, d_properties(original.d_properties, basicAllocator)
{
}

// MANIPULATORS
inline ListenOptions&
ListenOptions::setEndpoint(const bslstl::StringRef& value)
{
    d_endpoint = value;
    return *this;
}

inline bmqvt::PropertyBag& ListenOptions::properties()
{
    return d_properties;
}

// ACCESSORS
inline const bsl::string& ListenOptions::endpoint() const
{
    return d_endpoint;
}

inline const bmqvt::PropertyBag& ListenOptions::properties() const
{
    return d_properties;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& bmqio::operator<<(bsl::ostream&               stream,
                                       const bmqio::ListenOptions& obj)
{
    return obj.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
