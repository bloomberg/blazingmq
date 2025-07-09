// Copyright 2022-2023 Bloomberg Finance L.P.
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

// m_bmqtool_inpututil.cpp                                            -*-C++-*-
#include <m_bmqtool_inpututil.h>

// BMQ
#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_string.h>
#include <bdlb_tokenizer.h>
#include <bdlde_hexdecoder.h>
#include <bsl_cstdlib.h>
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>

namespace BloombergLP {
namespace m_bmqtool {

// ----------------
// struct InputUtil
// ----------------

bool InputUtil::getLine(bsl::string* out)
{
    BSLS_ASSERT_SAFE(out);

    bsl::cout << "> " << bsl::flush;
    bsl::cin.clear();
    bsl::getline(bsl::cin, *out);
    bdlb::String::trim(out);
    if (bsl::cin.eof()) {
        // User typed Ctrl-D
        bsl::cout << bsl::endl;
        return false;  // RETURN
    }
    return true;
}

void InputUtil::preprocessInput(bsl::string*                     verb,
                                bsl::string*                     output,
                                const bsl::string&               input,
                                bsl::unordered_set<bsl::string>* keys)
{
    BSLS_ASSERT_SAFE(verb);
    BSLS_ASSERT_SAFE(output);

    bmqu::MemOutStream oss;

    bool isKey = true, isFirstKey = true, isVerb = true;

    bdlb::Tokenizer iter(input, "= ");
    while (iter.isValid()) {
        if (isVerb) {
            *verb = iter.token();
            oss << '{';
            isVerb = false;
            ++iter;
            continue;  // CONTINUE
        }
        if (isKey) {
            if (!isFirstKey) {
                oss << ',';
            }
            if (iter.token()[0] != '"') {
                // stringify the key if needed
                if (iter.token() == "true" || iter.token() == "false") {
                    oss << iter.token();
                }
                else {
                    oss << '"' << iter.token() << '"';
                }
            }
            else {
                oss << iter.token();
            }
            if (keys) {
                keys->insert(iter.token());
            }
        }
        else {
            oss << ':';

            char start = iter.token()[0];
            char end   = 0;

            oss << iter.token();
            if (start == '[') {
                end = ']';
            }
            else if (start == '{') {
                end = '}';
            }
            else if (start == '"') {
                end = '"';
            }

            if (end) {
                while (iter.token()[iter.token().length() - 1] != end) {
                    ++iter;
                    if (!iter.isValid()) {
                        oss << end;
                        break;  // BREAK
                    }
                    oss << iter.previousDelimiter() << iter.token();
                }
                if (!iter.isValid()) {
                    // Exit from the outer loop if we exited from the inner
                    // loop because reached end of the input.
                    break;  // BREAK
                }
            }
        }
        isKey      = !isKey;
        isFirstKey = false;
        ++iter;
    }

    oss << '}';
    output->assign(oss.str().data(), oss.str().length());
}

void InputUtil::populateProperties(
    bmqa::MessageProperties*            out,
    const bsl::vector<MessageProperty>& properties)
{
    BSLS_ASSERT_SAFE(out);

    for (size_t i = 0; i < properties.size(); ++i) {
        const bsl::string& name  = properties[i].name();
        const bsl::string& value = properties[i].value();

        switch (properties[i].type()) {
        case MessagePropertyType::E_STRING: {
            BSLA_MAYBE_UNUSED int result = out->setPropertyAsString(name,
                                                                    value);
            BSLS_ASSERT_SAFE(0 == result);
        } break;  // BREAK

        case MessagePropertyType::E_INT: {
            BSLA_MAYBE_UNUSED int result =
                out->setPropertyAsInt32(name, stoi(value));
            BSLS_ASSERT_SAFE(0 == result);
        } break;  // BREAK
        default: BSLS_ASSERT_SAFE(false && "Unsupported type");
        }
    }
}

void InputUtil::verifyProperties(
    const bmqa::MessageProperties&      in,
    const bsl::vector<MessageProperty>& properties)
{
    if (properties.size() == 0) {
        // If no 'messageProperties' was specified on the command line, and
        // '_random' is present, assume certain producer logic

        bmqt::PropertyType::Enum type = bmqt::PropertyType::e_UNDEFINED;
        if (in.hasProperty("pairs_", &type)) {
            BSLS_ASSERT_SAFE(type == bmqt::PropertyType::e_INT32);

            BSLA_MAYBE_UNUSED int numPairs = in.getPropertyAsInt32("pairs_");

            BSLS_ASSERT_SAFE(in.numProperties() == (numPairs * 2 + 1));

            bmqa::MessagePropertiesIterator it(&in);

            bsl::unordered_set<bsl::string> pairs;

            pairs.insert("pairs_");

            while (it.hasNext()) {
                bsl::string name = it.name();

                if (pairs.find(name) != pairs.end()) {
                    continue;  // CONTINUE
                }

                name += "_value";
                BSLA_MAYBE_UNUSED bool hasValue = in.hasProperty(name, &type);

                BSLS_ASSERT_SAFE(hasValue);
                BSLS_ASSERT_SAFE(it.type() == type);

                pairs.insert(name);

                switch (type) {
                case bmqt::PropertyType::e_BOOL: {
                    BSLS_ASSERT_SAFE(it.getAsBool() ==
                                     in.getPropertyAsBool(name));
                    break;
                }
                case bmqt::PropertyType::e_CHAR: {
                    BSLS_ASSERT_SAFE(it.getAsChar() ==
                                     in.getPropertyAsChar(name));
                    break;
                }
                case bmqt::PropertyType::e_SHORT: {
                    BSLS_ASSERT_SAFE(it.getAsShort() ==
                                     in.getPropertyAsShort(name));
                    break;
                }
                case bmqt::PropertyType::e_INT32: {
                    BSLS_ASSERT_SAFE(it.getAsInt32() ==
                                     in.getPropertyAsInt32(name));
                    break;
                }
                case bmqt::PropertyType::e_INT64: {
                    BSLS_ASSERT_SAFE(it.getAsInt64() ==
                                     in.getPropertyAsInt64(name));
                    break;
                }
                case bmqt::PropertyType::e_STRING: {
                    BSLS_ASSERT_SAFE(it.getAsString() ==
                                     in.getPropertyAsString(name));
                    break;
                }
                case bmqt::PropertyType::e_BINARY: {
                    BSLS_ASSERT_SAFE(it.getAsBinary() ==
                                     in.getPropertyAsBinary(name));
                    break;
                }
                case bmqt::PropertyType::e_UNDEFINED:
                default:
                    BSLS_ASSERT_OPT(0 &&
                                    "Invalid data type for property value.");
                    break;  // BREAK
                }
            }
            return;  // RETURN
        }
    }
    BSLS_ASSERT_SAFE(size_t(in.numProperties()) >= properties.size());

    for (size_t i = 0; i < properties.size(); ++i) {
        const bsl::string&      name               = properties[i].name();
        BSLA_MAYBE_UNUSED const bsl::string& value = properties[i].value();

        switch (properties[i].type()) {
        case MessagePropertyType::E_STRING: {
            BSLA_MAYBE_UNUSED const bsl::string& result =
                in.getPropertyAsString(name);
            BSLS_ASSERT_SAFE(value == result);
        } break;  // BREAK

        case MessagePropertyType::E_INT: {
            BSLA_MAYBE_UNUSED int result = in.getPropertyAsInt32(name);
            BSLS_ASSERT_SAFE(stoi(value) == result);
        } break;  // BREAK
        default: BSLS_ASSERT_SAFE(false && "Unsupported type");
        }
    }
}

bool InputUtil::populateSubscriptions(bmqt::QueueOptions*              out,
                                      const bsl::vector<Subscription>& in,
                                      bslma::Allocator* allocator)
{
    BSLS_ASSERT_SAFE(out);

    bool failed = false;
    for (size_t i = 0; i < in.size(); ++i) {
        bmqt::Subscription             to;
        const Subscription&            from = in[i];
        const bmqt::SubscriptionHandle handle(
            from.correlationId().has_value()
                ? bmqt::CorrelationId(from.correlationId().value())
                : bmqt::CorrelationId());

        if (from.expression().has_value()) {
            bmqt::SubscriptionExpression expression(
                from.expression().value(),
                bmqt::SubscriptionExpression::e_VERSION_1);

            to.setExpression(expression);
        }

        if (from.maxUnconfirmedMessages().has_value()) {
            to.setMaxUnconfirmedMessages(
                from.maxUnconfirmedMessages().value());
        }
        else {
            to.setMaxUnconfirmedMessages(out->maxUnconfirmedMessages());
        }
        if (from.maxUnconfirmedBytes().has_value()) {
            to.setMaxUnconfirmedBytes(from.maxUnconfirmedBytes().value());
        }
        else {
            to.setMaxUnconfirmedBytes(out->maxUnconfirmedBytes());
        }
        if (from.consumerPriority().has_value()) {
            to.setConsumerPriority(from.consumerPriority().value());
        }
        else {
            to.setConsumerPriority(out->consumerPriority());
        }

        bsl::string error(allocator);
        if (!out->addOrUpdateSubscription(&error, handle, to)) {
            // It is possible to make early return here, but we want to log
            // all the failed expressions, not only the first failure.
            BALL_LOG_ERROR << "#INVALID_SUBSCRIPTION " << error;
            failed = true;
        }
    }
    return !failed;
}

bool InputUtil::populateSubscriptions(bmqt::QueueOptions* out,
                                      int                 autoPubSubModulo,
                                      const char*       autoPubSubPropertyName,
                                      bslma::Allocator* allocator)
{
    BSLS_ASSERT_SAFE(out);

    bool failed = false;
    for (int i = 0; i < autoPubSubModulo; ++i) {
        bmqt::Subscription       to;
        bmqt::CorrelationId      correlationId(i);
        bmqt::SubscriptionHandle handle(correlationId);

        bsl::string equality(autoPubSubPropertyName, allocator);
        equality += "==";
        equality += bsl::to_string(i);

        bmqt::SubscriptionExpression expression(
            equality,
            bmqt::SubscriptionExpression::e_VERSION_1);

        to.setExpression(expression);

        bsl::string error(allocator);
        if (!out->addOrUpdateSubscription(&error, handle, to)) {
            // It is possible to make early return here, but we want to log all
            // the failed expressions, not only the first failure.
            BALL_LOG_ERROR << "#INVALID_SUBSCRIPTION " << error;
            failed = true;
        }
    }
    return !failed;
}

bool InputUtil::decodeHexDump(bsl::ostream*     out,
                              bsl::ostream*     error,
                              bsl::istream&     in,
                              bslma::Allocator* allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(out);
    BSLS_ASSERT(error);

    char outputBuffer[4];  // should be equal to k_BLOCK_SIZE in
                           // bdlb::Print::hexDump()
    bdlde::HexDecoder hexDecoder;

    bsl::string line(allocator);
    int         numOut = 0;
    int         numIn  = 0;

    while (bsl::getline(in, line)) {
        if (line.empty()) {
            // Stop at the end of hexdump (empty line)
            break;  // BREAK
        }

        // Sanity check
        if (line.find(' ') == bsl::string::npos) {
            *error << "Wrong hexdump format, space delimeter is not detected";
            return false;  // RETURN
        }

        // Convert hexdump to binary, see format in bdlb::Print::hexDump()
        bdlb::Tokenizer           tokenizer(line, " ");
        bdlb::Tokenizer::iterator tokenizerIt = tokenizer.begin();
        // Process tokens skipping the first one (address offset)
        for (++tokenizerIt; tokenizerIt != tokenizer.end(); ++tokenizerIt) {
            bslstl::StringRef token = *tokenizerIt;

            // Stop when ASCII representation is detected
            if (token.front() == '|')
                break;  // BREAK

            // Check token size fits outputBuffer size
            if (token.size() > 2 * sizeof(outputBuffer)) {
                *error << "Wrong hexdump format, block size is greater than "
                       << 2 * sizeof(outputBuffer) << ": " << token.size();
                return false;  // RETURN
            }

            const int rc = hexDecoder.convert(outputBuffer,
                                              &numOut,
                                              &numIn,
                                              token.begin(),
                                              token.end());
            if (rc < 0) {
                *error << "HexDecoder convert error: " << rc;
                return false;  // RETURN
            }
            out->write(outputBuffer, numOut);
        }
    }

    return true;
}

bool InputUtil::loadMessageFromFile(bsl::ostream*      payload,
                                    bsl::ostream*      properties,
                                    bsl::ostream*      error,
                                    const bsl::string& filePath,
                                    bslma::Allocator*  allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(payload);
    BSLS_ASSERT(properties);
    BSLS_ASSERT(error);

    bsl::ifstream fileStream(filePath.c_str());
    if (!fileStream.is_open()) {
        *error << "Failed to open file: " << filePath;
        return false;  // RETURN
    }

    // Parse file according to format defined in
    // QueueEngineUtil::dumpMessageInTempfile()
    char        tmpBuffer[2];
    bsl::string line(allocator);
    bsl::getline(fileStream, line);

    // Check if properties are present
    if (line == "Message Properties:") {
        fileStream.read(tmpBuffer, 1);  // skip empty line

        // Read human readable properties lines to check surrounding
        // markers [
        // ]
        bsl::getline(fileStream, line);
        if (line.front() != '[') {
            *error << "Properties '[' marker missed";
            return false;  // RETURN
        }
        if (line.back() != ']') {
            // Binary properties are multiline, read lines until close
            // marker
            // ']'
            while (!fileStream.eof()) {
                bsl::getline(fileStream, line);
                // Check for close marker
                if (line.back() == ']')
                    break;  // BREAK
            }
            if (fileStream.eof()) {
                *error << "Properties ']' marker missed";
                return false;  // RETURN
            }
        }

        fileStream.read(tmpBuffer, 2);  // skip empty lines

        // Check if properties hexdump is present
        bsl::getline(fileStream, line);
        if (line != "Message Properties hexdump:") {
            *error << "Unexpected file format, 'Message Properties hexdump:' "
                      "expected";
            return false;  // RETURN
        }

        fileStream.read(tmpBuffer, 1);  // skip empty line
        // Read and convert message properties
        if (!InputUtil::decodeHexDump(properties,
                                      error,
                                      fileStream,
                                      allocator)) {
            return false;  // RETURN
        }

        // Check message payload presence
        fileStream.read(tmpBuffer, 1);  // skip empty line
        bsl::getline(fileStream, line);
        if (line != "Message Payload:") {
            *error << "Unexpected file format, 'Message Payload:' expected";
            return false;  // RETURN
        }
    }
    else if (line != "Application Data:") {
        *error << "Unexpected file format, either 'Message Properties:' or "
                  "'Application Data:' expected";
        return false;  // RETURN
    }

    // Read and convert message payload
    fileStream.read(tmpBuffer, 1);  // skip empty line
    if (!InputUtil::decodeHexDump(payload, error, fileStream, allocator)) {
        return false;  // RETURN
    }

    return true;
}

}  // close package namespace
}  // close enterprise namespace
