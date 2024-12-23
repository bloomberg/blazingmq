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

// bmqstoragetool
#include <m_bmqstoragetool_compositesequencenumber.h>

// BDE
#include <bdlb_print.h>
#include <bsl_stdexcept.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =============================
// class CompositeSequenceNumber
// =============================

CompositeSequenceNumber::CompositeSequenceNumber()
: d_leaseId(0)
, d_seqNumber(0)
, d_isSet(false)
{
    // NOTHING
}

CompositeSequenceNumber::CompositeSequenceNumber(
    const bsls::Types::Uint64 leaseId,
    const bsls::Types::Uint64 sequenceNumber)
: d_leaseId(leaseId)
, d_seqNumber(sequenceNumber)
{
    BSLS_ASSERT(d_leaseId > 0 && d_seqNumber > 0);
    d_isSet = d_leaseId > 0 && d_seqNumber > 0;
}

CompositeSequenceNumber&
CompositeSequenceNumber::fromString(bsl::ostream&      errorDescription,
                                    const bsl::string& seqNumString)
{
    d_isSet = false;

    if (seqNumString.empty()) {
        errorDescription << "Invalid input: empty string.";
        return *this;  // RETURN
    }

    // Find the position of the separator
    const size_t separatorPos = seqNumString.find('-');
    if (separatorPos == bsl::string::npos) {
        errorDescription << "Invalid format: no '-' separator found.";
        return *this;  // RETURN
    }

    // Extract parts
    const bsl::string firstPart  = seqNumString.substr(0, separatorPos);
    const bsl::string secondPart = seqNumString.substr(separatorPos + 1);

    // Convert parts to numbers
    try {
        size_t posFirst, posSecond;

        d_leaseId   = bsl::stoul(firstPart, &posFirst);
        d_seqNumber = bsl::stoul(secondPart, &posSecond);

        if (posFirst != firstPart.size() || posSecond != secondPart.size()) {
            throw bsl::invalid_argument("");  // THROW
        }

        if (d_leaseId == 0 || d_seqNumber == 0) {
            errorDescription << "Invalid input: zero values encountered.";
            return *this;  // RETURN
        }

        d_isSet = true;
    }
    catch (const bsl::invalid_argument& e) {
        errorDescription << "Invalid input: non-numeric values encountered.";
    }
    catch (const bsl::out_of_range& e) {
        errorDescription << "Invalid input: number out of range.";
    }

    return *this;
}

bsl::ostream& CompositeSequenceNumber::print(bsl::ostream& stream,
                                             int           level,
                                             int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);

    if (isSet()) {
        stream << "leaseId: " << leaseId()
               << ", sequenceNumber: " << sequenceNumber();
    }
    else {
        stream << "** UNSET **";
    }

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
