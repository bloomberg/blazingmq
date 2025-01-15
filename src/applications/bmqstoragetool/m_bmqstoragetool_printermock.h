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

#ifndef INCLUDED_M_BMQSTORAGETOOL_PRINTERMOCK_H
#define INCLUDED_M_BMQSTORAGETOOL_PRINTERMOCK_H

//@PURPOSE: Provide mock implementation of Printer for unit tests.
//
//@CLASSES:
//  PrinterMock: provides Printer implementation with mocked methods.
//
//@DESCRIPTION: Printer mock for unit testing.

// bmqstoragetool
#include <m_bmqstoragetool_printer.h>

// GMOCK
// If bmqst_testhelper.h was defined before gtest.h, preserve macroses values.
// If not, undefine values from gtest.h.
#pragma push_macro("ASSERT_EQ")
#pragma push_macro("ASSERT_NE")
#pragma push_macro("ASSERT_LT")
#pragma push_macro("ASSERT_LE")
#pragma push_macro("ASSERT_GT")
#pragma push_macro("ASSERT_GE")
#pragma push_macro("TEST_F")
#pragma push_macro("TEST")

#include <gmock/gmock.h>

#undef ASSERT_EQ
#undef ASSERT_NE
#undef ASSERT_LT
#undef ASSERT_LE
#undef ASSERT_GT
#undef ASSERT_GE
#undef TEST_F
#undef TEST
#pragma pop_macro("ASSERT_EQ")
#pragma pop_macro("ASSERT_NE")
#pragma pop_macro("ASSERT_LT")
#pragma pop_macro("ASSERT_LE")
#pragma pop_macro("ASSERT_GT")
#pragma pop_macro("ASSERT_GE")
#pragma pop_macro("TEST_F")
#pragma pop_macro("TEST")

namespace BloombergLP {

namespace m_bmqstoragetool {

// =================
// class PrinterMock
// =================

class PrinterMock : public Printer {
  public:
    // CREATORS
    PrinterMock() {}

    ~PrinterMock() BSLS_KEYWORD_OVERRIDE {}

    // PUBLIC METHODS

    MOCK_CONST_METHOD1(printMessage, void(const MessageDetails&));
    MOCK_CONST_METHOD1(printGuidNotFound, void(const bmqt::MessageGUID&));
    MOCK_CONST_METHOD1(printGuid, void(const bmqt::MessageGUID&));
    MOCK_CONST_METHOD1(printFooter, void(bsl::size_t));
    MOCK_CONST_METHOD1(printOutstandingRatio,
                       void(const OutstandingPrintBundle&));
    MOCK_CONST_METHOD3(printSummary,
                       void(bsl::size_t, bsl::size_t, bsl::size_t));
    MOCK_CONST_METHOD2(printJournalFileMeta,
                       void(const mqbs::JournalFileIterator*,
                            bslma::Allocator*));
    MOCK_CONST_METHOD1(printDataFileMeta, void(const mqbs::DataFileIterator*));
    MOCK_CONST_METHOD1(printGuidsNotFound, void(const GuidsList& guids));
};

}  // close package namespace

}  // close enterprise namespace

#endif  // INCLUDED_M_BMQSTORAGETOOL_PRINTERMOCK_H
