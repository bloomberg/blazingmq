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

#ifndef INCLUDED_M_BMQSTORAGETOOL_PRINTER_H
#define INCLUDED_M_BMQSTORAGETOOL_PRINTER_H

//@PURPOSE: Provide Printer class for printing storage files.
//
//@CLASSES:
//  Printer: provides methods to print storage files.
//
//@DESCRIPTION: Interface class to print storage files.

// bmqstoragetool
#include <m_bmqstoragetool_messagedetails.h>
#include <m_bmqstoragetool_parameters.h>

// BMQ
#include <bmqt_messageguid.h>

// BDE
#include <bsl_list.h>
#include <bslma_managedptr.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

// =============================
// struct OutstandingPrintBundle
// =============================

struct OutstandingPrintBundle {
    // DATA

    bsl::size_t d_totalMessagesCount;
    bsl::size_t d_outstandingMessages;
    int         d_ratio;

    // CREATORS

    OutstandingPrintBundle(bsl::size_t totalMessagesCount,
                           bsl::size_t outstandingMessages,
                           int         ratio)
    : d_totalMessagesCount(totalMessagesCount)
    , d_outstandingMessages(outstandingMessages)
    , d_ratio(ratio)
    {
    }

    // OPERATORS

    bool operator==(OutstandingPrintBundle const& other) const
    {
        return d_totalMessagesCount == other.d_totalMessagesCount &&
               d_outstandingMessages == other.d_outstandingMessages &&
               d_ratio == other.d_ratio;
    }
};

// =============
// class Printer
// =============

class Printer {
  protected:
    // PROTECTED TYPES

    typedef bsl::list<bmqt::MessageGUID> GuidsList;
    // List of message guids.

  public:
    // CREATORS

    virtual ~Printer() {}

    // PUBLIC METHODS

    virtual void printMessage(const MessageDetails& details) const = 0;

    virtual void printGuid(const bmqt::MessageGUID& guid) const = 0;

    virtual void printGuidNotFound(const bmqt::MessageGUID& guid) const = 0;

    virtual void printGuidsNotFound(const GuidsList& guids) const = 0;

    virtual void printFooter(bsl::size_t foundMessagesCount) const = 0;

    virtual void
    printOutstandingRatio(const OutstandingPrintBundle& bundle) const = 0;

    virtual void printSummary(bsl::size_t foundMessagesCount,
                              bsl::size_t deletedMessagesCount,
                              bsl::size_t partiallyConfirmedCount) const = 0;

    virtual void
    printJournalFileMeta(const mqbs::JournalFileIterator* journalFile_p,
                         bslma::Allocator*                allocator) const = 0;

    virtual void
    printDataFileMeta(const mqbs::DataFileIterator* dataFile_p) const = 0;
};

bsl::shared_ptr<Printer> createPrinter(Parameters::PrintMode mode,
                                       std::ostream&         stream,
                                       bslma::Allocator*     allocator);

}  // close package namespace

}  // close enterprise namespace

#endif  // INCLUDED_M_BMQSTORAGETOOL_PRINTER_H
