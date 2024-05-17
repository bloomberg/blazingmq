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
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_string.h>
#include <bdlb_tokenizer.h>
#include <bdlde_hexdecoder.h>
#include <bsl_cstdlib.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace m_bmqtool {

namespace {

// Helper to validate properties string format
bool validatePropertiesStr(bsl::ostream* error, const bsl::string& properties)
{
    bool result = true;

    if (properties.empty()) {
        if (error) {
            *error << "Unexpected empty properties string, use empty brackets "
                      "'[ ]' instead";
        }
        result = false;
    }
    else if (properties.front() != '[') {
        if (error) {
            *error << "Expected open marker '[]' missed";
        }
        result = false;
    }
    else if (properties.back() != ']') {
        if (error) {
            *error << "Expected close marker ']' missed";
        }
        result = false;
    }

    return result;
}

}  // close unnamed namespace

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

    mwcu::MemOutStream oss;

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
            const int rc = out->setPropertyAsString(name, value);
            BSLS_ASSERT(0 == rc);
        } break;  // BREAK

        case MessagePropertyType::E_INT32: {
            BSLA_MAYBE_UNUSED const int rc =
                out->setPropertyAsInt32(name, bsl::stoi(value));
            BSLS_ASSERT(0 == rc);
        } break;  // BREAK

        case MessagePropertyType::E_INT64: {
            BSLA_MAYBE_UNUSED const int rc =
                out->setPropertyAsInt64(name, bsl::stoll(value));
            BSLS_ASSERT(0 == rc);
        } break;  // BREAK

        case MessagePropertyType::E_BOOL: {
            bool                         boolValue;
            bsl::stringstream            iss(value);
            BSLA_MAYBE_UNUSED const bool readSuccess =
                (iss >> bsl::boolalpha >> boolValue).good();
            BSLS_ASSERT(readSuccess);
            BSLA_MAYBE_UNUSED const int rc = out->setPropertyAsBool(name,
                                                                    boolValue);
            BSLS_ASSERT(0 == rc);
        } break;  // BREAK

        case MessagePropertyType::E_CHAR: {
            // Char value is represented in hex, convert it to int and check
            // range
            const int intVal = bsl::stoi(value, 0, 16);
            BSLS_ASSERT(intVal <= bsl::numeric_limits<char>::max());
            BSLS_ASSERT(intVal >= bsl::numeric_limits<char>::min());
            BSLA_MAYBE_UNUSED const int rc =
                out->setPropertyAsChar(name, static_cast<char>(intVal));
            BSLS_ASSERT(0 == rc);
        } break;  // BREAK

        case MessagePropertyType::E_SHORT: {
            const int intVal = bsl::stoi(value);
            BSLS_ASSERT(intVal <= bsl::numeric_limits<short>::max());
            BSLS_ASSERT(intVal >= bsl::numeric_limits<short>::min());
            BSLA_MAYBE_UNUSED const int rc =
                out->setPropertyAsShort(name, static_cast<short>(intVal));
            BSLS_ASSERT(0 == rc);
        } break;  // BREAK

        case MessagePropertyType::E_BINARY: {
            bsl::istringstream iss(value);
            mwcu::MemOutStream resultStream;
            const bool decodeSuccess = InputUtil::decodeHexDump(&resultStream,
                                                                iss);
            BSLS_ASSERT(decodeSuccess);
            bsl::vector<char>     binaryBuf(resultStream.str().begin(),
                                        resultStream.str().end());
            BSLA_MAYBE_UNUSED int rc = out->setPropertyAsBinary(name,
                                                                binaryBuf);
            BSLS_ASSERT(0 == rc);

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

            int numPairs = in.getPropertyAsInt32("pairs_");

            BSLS_ASSERT_SAFE(in.numProperties() == (numPairs * 2 + 1));

            (void)numPairs;
            bmqa::MessagePropertiesIterator it(&in);

            bsl::unordered_set<bsl::string> pairs;

            pairs.insert("pairs_");

            while (it.hasNext()) {
                bsl::string name = it.name();

                if (pairs.find(name) != pairs.end()) {
                    continue;  // CONTINUE
                }

                name += "_value";
                bool hasValue = in.hasProperty(name, &type);

                BSLS_ASSERT_SAFE(hasValue);
                (void)hasValue;
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

        case MessagePropertyType::E_INT32: {
            BSLA_MAYBE_UNUSED const int result = in.getPropertyAsInt32(name);
            BSLS_ASSERT_SAFE(stoi(value) == result);
        } break;  // BREAK

        case MessagePropertyType::E_INT64: {
            BSLA_MAYBE_UNUSED const bsls::Types::Int64 result =
                in.getPropertyAsInt64(name);
            BSLS_ASSERT_SAFE(stoll(value) == result);
        } break;  // BREAK
        case MessagePropertyType::E_CHAR: {
            BSLA_MAYBE_UNUSED const char result = in.getPropertyAsChar(name);
            BSLS_ASSERT_SAFE(static_cast<char>(bsl::stoi(value, 0, 16)) ==
                             result);
        } break;  // BREAK
        case MessagePropertyType::E_SHORT: {
            BSLA_MAYBE_UNUSED const short result = in.getPropertyAsShort(name);
            BSLS_ASSERT_SAFE(static_cast<short>(bsl::stoi(value)) == result);
        } break;  // BREAK

        case MessagePropertyType::E_BINARY: {
            bsl::istringstream           iss(value);
            mwcu::MemOutStream           resultStream;
            BSLA_MAYBE_UNUSED const bool decodeSuccess =
                InputUtil::decodeHexDump(&resultStream, iss);
            BSLS_ASSERT_SAFE(true == decodeSuccess);
            bsl::vector<char>       binaryBuf(resultStream.str().begin(),
                                        resultStream.str().end());
            BSLA_MAYBE_UNUSED const bsl::vector<char>& result =
                in.getPropertyAsBinary(name);
            BSLS_ASSERT_SAFE(binaryBuf == result);
        } break;  // BREAK

        default: BSLS_ASSERT_SAFE(false && "Unsupported type");
        }
    }
}

bool InputUtil::parseProperties(bsl::vector<MessageProperty>* out,
                                const bsl::string&            properties,
                                bsl::ostream*                 error)
{
    // PRECONDITIONS
    BSLS_ASSERT(out);

    // Parsing state which is changing circularly in the following order:
    // e_NAME -> e_TYPE -> e_VALUE -> e_NAME -> ...
    enum State { e_NAME, e_TYPE, e_VALUE };

    // Sanity check
    if (!validatePropertiesStr(error, properties)) {
        return false;  // RETURN
    }

    // Tokenize string by space and check markers
    bdlb::Tokenizer           tokenizer(properties, " ");
    bdlb::Tokenizer::iterator tokenizerIt = tokenizer.begin();
    MessageProperty           messageProperty;
    State                     state = e_NAME;
    // Process tokens skipping open marker
    for (++tokenizerIt; tokenizerIt != tokenizer.end(); ++tokenizerIt) {
        bslstl::StringRef token = *tokenizerIt;

        if (token == "=" || token == "]")
            continue;

        switch (state) {
        case e_NAME: {
            messageProperty.name() = token;
            state                  = e_TYPE;
        } break;  // BREAK
        case e_TYPE: {
            // Add enum prefix, remove surrounding brackets, and convert to
            // MessagePropertyType enum
            bsl::string typeStr = "E_";
            typeStr.append(token.substr(1, token.size() - 2));
            if (MessagePropertyType::fromString(&messageProperty.type(),
                                                typeStr) != 0) {
                if (error) {
                    *error << "Failed to decode MessagePropertyType: "
                           << token;
                }
                return false;  // RETURN
            }
            state = e_VALUE;
        } break;  // BREAK
        case e_VALUE: {
            // Special handling for binary value
            if (messageProperty.type() ==
                MessagePropertyType::Value::E_BINARY) {
                // Check opening quote mark presense
                if (token.front() != '"') {
                    if (error) {
                        *error << "Binary value opening quote mark missed";
                    }
                    return false;  // RETURN
                }
                // Join tokens by space (detokenize) inside surrounding quotes.
                // Quote mark after end of the line is the end of hexdump.
                mwcu::MemOutStream resultStream(' ');
                // Process tokens skipping opening quote mark
                for (++tokenizerIt; tokenizerIt != tokenizer.end();
                     ++tokenizerIt) {
                    token = *tokenizerIt;
                    // Check closing quote mark after end line
                    if (token.find("|\n\"") != bsl::string::npos) {
                        break;  // BREAK
                    }

                    resultStream << token << ' ';
                }

                messageProperty.value() = resultStream.str();
            }
            else if (messageProperty.type() ==
                     MessagePropertyType::Value::E_STRING) {
                // Special handling for string value: it may contain spaces
                // Join tokens by space (detokenize) inside surrounding quotes.
                mwcu::MemOutStream resultStream;
                for (; tokenizerIt != tokenizer.end(); ++tokenizerIt) {
                    token = *tokenizerIt;
                    resultStream << token;
                    if (token.back() == '"') {
                        break;  // BREAK
                    }
                    resultStream << ' ';
                }
                // Remove surrounding quotes and save
                messageProperty.value() = resultStream.str().substr(
                    1,
                    resultStream.str().size() - 2);
            }
            else {
                messageProperty.value() = token;
            }
            // Property is parsed, save it
            out->push_back(messageProperty);
            state = e_NAME;
        } break;  // BREAK
        default: {
            BSLS_ASSERT_SAFE(false && "Unsupported parsing state");
        }
        }
    }

    return true;
}

bool InputUtil::populateSubscriptions(bmqt::QueueOptions*              out,
                                      const bsl::vector<Subscription>& in)
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

        bsl::string error;
        if (!out->addOrUpdateSubscription(&error, handle, to)) {
            // It is possible to make early return here, but we want to log all
            // the failed expressions, not only the first failure.
            BALL_LOG_ERROR << "#INVALID_SUBSCRIPTION " << error;
            failed = true;
        }
    }
    return !failed;
}

bool InputUtil::decodeHexDump(bsl::ostream* out,
                              bsl::istream& in,
                              bsl::ostream* error)
{
    // PRECONDITIONS
    BSLS_ASSERT(out);

    char outputBuffer[4];  // should be equal to k_BLOCK_SIZE in
                           // bdlb::Print::hexDump()
    bdlde::HexDecoder hexDecoder;

    bsl::string line;
    int         numOut = 0;
    int         numIn  = 0;

    while (bsl::getline(in, line)) {
        if (line.empty()) {
            continue;  // skip empty lines
        }

        // Sanity check
        if (line.find(' ') == bsl::string::npos) {
            if (error) {
                *error
                    << "Wrong hexdump format, space delimeter is not detected";
            }
            return false;  // RETURN
        }

        // Convert hexdump to binary, see format in bdlb::Print::hexDump()
        bdlb::Tokenizer           tokenizer(line, " ");
        bdlb::Tokenizer::iterator tokenizerIt = tokenizer.begin();
        // Process tokens skipping the first one (address offset)
        for (++tokenizerIt; tokenizerIt != tokenizer.end(); ++tokenizerIt) {
            bslstl::StringRef token = *tokenizerIt;

            // Stop when ASCII representation is detected
            if (token.at(0) == '|')
                break;  // BREAK

            const int rc = hexDecoder.convert(outputBuffer,
                                              &numOut,
                                              &numIn,
                                              token.begin(),
                                              token.end());
            if (rc < 0) {
                if (error) {
                    *error << "HexDecoder convert error: " << rc;
                }
                return false;  // RETURN
            }
            out->write(outputBuffer, numOut);
        }
    }

    return true;
}

}  // close package namespace
}  // close enterprise namespace
