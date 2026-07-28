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

// BMQ
#include <bdlbb_blob.h>
#include <bmqp_messageproperties.h>
#include <bmqu_memoutstream.h>
#include <bsl_algorithm.h>
#include <bsl_ostream.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

const unsigned int k_MAX_PAYLOAD_HEX_BYTES = 64;

int seekToRecord(mqbs::DataFileIterator* it, bsls::Types::Uint64 recordOffset)
{
    if ((it->recordOffset() > recordOffset && !it->isReverseMode()) ||
        (it->recordOffset() < recordOffset && it->isReverseMode())) {
        it->flipDirection();
    }
    while (it->recordOffset() != recordOffset) {
        const int rc = it->nextRecord();
        if (rc != 1) {
            return -1;
        }
    }
    return 0;
}

void loadAppData(const char**            appData,
                 unsigned int*           appDataLen,
                 unsigned int*           propertiesAreaLen,
                 mqbs::DataFileIterator* it)
{
    *propertiesAreaLen = 0;
    it->loadApplicationData(appData, appDataLen);

    const mqbs::DataHeader& dh = it->dataHeader();
    if (mqbs::DataHeaderFlagUtil::isSet(
            dh.flags(),
            mqbs::DataHeaderFlags::e_MESSAGE_PROPERTIES)) {
        bsl::shared_ptr<char> sp(const_cast<char*>(*appData),
                                 bslstl::SharedPtrNilDeleter(),
                                 0);
        bdlbb::BlobBuffer     buf(sp, *appDataLen);
        bdlbb::Blob           blob;
        blob.appendDataBuffer(buf);

        bmqp::MessageProperties props;
        if (0 ==
            props.streamIn(blob,
                           bmqp::MessagePropertiesInfo(dh).isExtended())) {
            *propertiesAreaLen = props.totalSize();
            *appDataLen -= *propertiesAreaLen;
            *appData += *propertiesAreaLen;
        }
    }
}

}  // close unnamed namespace

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

    if (seekToRecord(d_dataFile_p, recordOffset) != 0) {
        d_ostream << "Failed to retrieve message from DATA file" << bsl::endl;
        return;
    }

    bmqu::MemOutStream dataHeaderOsstr(d_allocator_p);
    bmqu::MemOutStream optionsOsstr(d_allocator_p);
    bmqu::MemOutStream propsOsstr(d_allocator_p);

    dataHeaderOsstr << d_dataFile_p->dataHeader();

    const char*  options    = 0;
    unsigned int optionsLen = 0;
    d_dataFile_p->loadOptions(&options, &optionsLen);
    mqbs::FileStoreProtocolPrinter::printOptions(optionsOsstr,
                                                 options,
                                                 optionsLen);

    const char*  appData           = 0;
    unsigned int appDataLen        = 0;
    unsigned int propertiesAreaLen = 0;
    loadAppData(&appData, &appDataLen, &propertiesAreaLen, d_dataFile_p);

    const mqbs::DataHeader& dh = d_dataFile_p->dataHeader();
    if (mqbs::DataHeaderFlagUtil::isSet(
            dh.flags(),
            mqbs::DataHeaderFlags::e_MESSAGE_PROPERTIES)) {
        const char*  propsData = 0;
        unsigned int propsLen  = 0;
        d_dataFile_p->loadApplicationData(&propsData, &propsLen);
        int rc = mqbs::FileStoreProtocolPrinter::printMessageProperties(
            &propertiesAreaLen,
            propsOsstr,
            propsData,
            bmqp::MessagePropertiesInfo(dh));
        if (rc) {
            d_ostream << "Failed to retrieve message properties, rc: " << rc
                      << bsl::endl;
        }
    }

    // Payload
    bmqu::MemOutStream payloadOsstr(d_allocator_p);
    unsigned int minLen = d_dumpLimit > 0 ? bsl::min(appDataLen, d_dumpLimit)
                                          : appDataLen;
    payloadOsstr << "First " << minLen << " bytes of payload:" << '\n';
    bdlb::Print::hexDump(payloadOsstr, appData, minLen);
    if (minLen < appDataLen) {
        payloadOsstr << "And " << (appDataLen - minLen)
                     << " more bytes (redacted)" << '\n';
    }

    d_ostream << '\n'
              << "DataRecord index: " << d_dataFile_p->recordIndex()
              << ", offset: " << d_dataFile_p->recordOffset() << '\n'
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

int PayloadDumper::loadPayloadHex(bsl::string*        result,
                                  bsls::Types::Uint64 messageOffsetDwords)
{
    const bsls::Types::Uint64 recordOffset = messageOffsetDwords *
                                             bmqp::Protocol::k_DWORD_SIZE;

    if (seekToRecord(d_dataFile_p, recordOffset) != 0) {
        return -1;
    }

    const char*  appData           = 0;
    unsigned int appDataLen        = 0;
    unsigned int propertiesAreaLen = 0;
    loadAppData(&appData, &appDataLen, &propertiesAreaLen, d_dataFile_p);

    unsigned int len = bsl::min(appDataLen, k_MAX_PAYLOAD_HEX_BYTES);
    result->clear();
    result->reserve(len * 2);
    for (unsigned int i = 0; i < len; ++i) {
        const char    hexDigits[] = "0123456789ABCDEF";
        unsigned char byte        = static_cast<unsigned char>(appData[i]);
        result->push_back(hexDigits[byte >> 4]);
        result->push_back(hexDigits[byte & 0x0F]);
    }

    return 0;
}

}  // close package namespace
}  // close enterprise namespace
