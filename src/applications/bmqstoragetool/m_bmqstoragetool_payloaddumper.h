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

// m_bmqstoragetool_payloaddumper.h -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_PAYLOADDUMPER
#define INCLUDED_M_BMQSTORAGETOOL_PAYLOADDUMPER

//@PURPOSE: Provide a message payload dumping.
//
//@CLASSES:
//  m_bmqstoragetool::PayloadDumper: dump message payload.
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
    // DATA
    bsl::ostream&           d_ostream;
    mqbs::DataFileIterator* d_dataFile_p;
    unsigned int            d_dumpLimit;

  public:
    // CREATORS
    explicit PayloadDumper(bsl::ostream&           ostream,
                           mqbs::DataFileIterator* dataFile_p,
                           unsigned int            dumpLimit);

    void outputPayload(bsls::Types::Uint64 messageOffsetDwords);
};

}  // close package namespace
}  // close enterprise namespace

#endif
