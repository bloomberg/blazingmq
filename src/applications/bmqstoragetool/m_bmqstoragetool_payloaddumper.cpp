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
#include <m_bmqstoragetool_payloaddumper.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>

// MWC
#include <mwcu_memoutstream.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// ===================
// class PayloadDumper
// ===================

PayloadDumper::PayloadDumper(bsl::ostream&           ostream,
                             mqbs::DataFileIterator* dataFile_p,
                             unsigned int            dumpLimit,
                             bslma::Allocator*       allocator)
: d_ostream(ostream)
, d_dataFile_p(dataFile_p)
, d_dumpLimit(dumpLimit)
, d_allocator_p(allocator)
{
    // NOTHING
}

void PayloadDumper::outputPayload(bsls::Types::Uint64 messageOffsetDwords)
{
    const bsls::Types::Uint64 recordOffset = messageOffsetDwords *
                                             bmqp::Protocol::k_DWORD_SIZE;
    mqbs::DataFileIterator* it = d_dataFile_p;

    // Flip iterator direction depending on recordOffset
    if ((it->recordOffset() > recordOffset && !it->isReverseMode()) ||
        (it->recordOffset() < recordOffset && it->isReverseMode())) {
        it->flipDirection();
    }
    // Search record in data file
    while (it->recordOffset() != recordOffset) {
        const int rc = it->nextRecord();

        if (rc != 1) {
            d_ostream << "Failed to retrieve message from DATA "
                      << "file rc: " << rc << bsl::endl;
            return;  // RETURN
        }
    }

    mwcu::MemOutStream dataHeaderOsstr(d_allocator_p);
    mwcu::MemOutStream optionsOsstr(d_allocator_p);
    mwcu::MemOutStream propsOsstr(d_allocator_p);

    dataHeaderOsstr << it->dataHeader();

    const char*  options    = 0;
    unsigned int optionsLen = 0;
    it->loadOptions(&options, &optionsLen);
    mqbs::FileStoreProtocolPrinter::printOptions(optionsOsstr,
                                                 options,
                                                 optionsLen);

    const char*  appData    = 0;
    unsigned int appDataLen = 0;
    it->loadApplicationData(&appData, &appDataLen);

    const mqbs::DataHeader& dh                = it->dataHeader();
    unsigned int            propertiesAreaLen = 0;
    if (mqbs::DataHeaderFlagUtil::isSet(
            dh.flags(),
            mqbs::DataHeaderFlags::e_MESSAGE_PROPERTIES)) {
        int rc = mqbs::FileStoreProtocolPrinter::printMessageProperties(
            &propertiesAreaLen,
            propsOsstr,
            appData,
            bmqp::MessagePropertiesInfo(dh));
        if (rc) {
            d_ostream << "Failed to retrieve message properties, rc: " << rc
                      << bsl::endl;
        }

        appDataLen -= propertiesAreaLen;
        appData += propertiesAreaLen;
    }

    // Payload
    mwcu::MemOutStream payloadOsstr(d_allocator_p);
    unsigned int minLen = d_dumpLimit > 0 ? bsl::min(appDataLen, d_dumpLimit)
                                          : appDataLen;
    payloadOsstr << "First " << minLen << " bytes of payload:" << '\n';
    bdlb::Print::hexDump(payloadOsstr, appData, minLen);
    if (minLen < appDataLen) {
        payloadOsstr << "And " << (appDataLen - minLen)
                     << " more bytes (redacted)" << '\n';
    }

    d_ostream << '\n'
              << "DataRecord index: " << it->recordIndex()
              << ", offset: " << it->recordOffset() << '\n'
              << "DataHeader: " << '\n'
              << dataHeaderOsstr.str();

    if (0 != optionsLen) {
        d_ostream << "\nOptions: " << '\n' << optionsOsstr.str();
    }

    if (0 != propertiesAreaLen) {
        d_ostream << "\nProperties: " << '\n' << propsOsstr.str();
    }

    d_ostream << "\nPayload: " << '\n' << payloadOsstr.str() << '\n';
}

}  // close package namespace
}  // close enterprise namespace
