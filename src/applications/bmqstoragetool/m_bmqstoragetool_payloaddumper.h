// Copyright 2014-2023 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGETOOL_PAYLOADDUMPER
#define INCLUDED_M_BMQSTORAGETOOL_PAYLOADDUMPER

//@PURPOSE: Provide a message payload dumping.
//
//@CLASSES:
//  m_bmqstoragetool::PayloadDumper: dumps message payload to output stream.
//
//@DESCRIPTION: 'PayloadDumper' provides a message payload dumping.

// MQB
#include <mqbs_datafileiterator.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// ===================
// class PayloadDumper
// ===================
class PayloadDumper {
  private:
    // PRIVATE DATA

    bsl::ostream& d_ostream;
    // Reference to output stream.
    mqbs::DataFileIterator* d_dataFile_p;
    // Pointer to data file iterator.
    unsigned int d_dumpLimit;
    // Dump limit value.
    bslma::Allocator* d_allocator_p;
    // Pointer to allocator that is used inside the class.

  public:
    // CREATORS

    /// Constructor using the specified `ostream`, `dataFile_p` and
    /// `dumpLimit`.
    PayloadDumper(bsl::ostream&           ostream,
                  mqbs::DataFileIterator* dataFile_p,
                  unsigned int            dumpLimit,
                  bslma::Allocator*       allocator);

    // MANIPULATORS

    /// Output message payload from the specified offset in Data file.
    void outputPayload(bsls::Types::Uint64 messageOffsetDwords);
};

}  // close package namespace
}  // close enterprise namespace

#endif
