// Copyright 2024 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGETOOL_PRINTMANAGER_H
#define INCLUDED_M_BMQSTORAGETOOL_PRINTMANAGER_H

//@PURPOSE: Provide PrintManager class for printing records.
//
//@CLASSES:
//  PrintManager: provides methods to print journal records.
//
//@DESCRIPTION: Interface class to print journal records.

// bmqstoragetool
#include <m_bmqstoragetool_messagedetails.h>
#include <m_bmqstoragetool_parameters.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

// ==================
// class PrintManager
// ==================

class PrintManager {
  public:
    // CREATORS

    virtual ~PrintManager() {}

    // PUBLIC METHODS

    virtual void printMessage(const MessageDetails& details) = 0;
    virtual void printFooter(bsl::size_t foundMessagesCount) = 0;
    virtual void printError(const bmqt::MessageGUID& guid)   = 0;
    virtual void printGuid(const bmqt::MessageGUID& guid)    = 0;
};

bslma::ManagedPtr<PrintManager>
createPrintManager(Parameters::PrintMode mode,
                   std::ostream&         stream,
                   bslma::Allocator*     allocator);

}  // close package namespace

}  // close enterprise namespace

#endif  // INCLUDED_M_BMQSTORAGETOOL_PRINTMANAGER_H
