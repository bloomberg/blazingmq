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

// m_bmqtool_inpututil.h                                              -*-C++-*-
#ifndef INCLUDED_M_BMQTOOL_INPUTUTIL
#define INCLUDED_M_BMQTOOL_INPUTUTIL

//@PURPOSE: Provide utility routines for input processing for bmqtool.
//
//@CLASSES:
//  m_bmqtool::InputUtil: Input utility routines for bmqtool.
//
//@DESCRIPTION: 'm_bmqtool::InputUtil' provides input utility routines for
// bmqtool.

// bmqtool
#include <m_bmqtool_messages.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <ball_log.h>
#include <bsl_string.h>

// BMQ
#include <bmqa_messageproperties.h>
#include <bmqt_queueoptions.h>

namespace BloombergLP {
namespace m_bmqtool {

// ================
// struct InputUtil
// ================

/// Input utility routines for bmqtool.
struct InputUtil {
    /// Read a line from stdin, using the standard `cin.getline` and save it
    /// in the specified `out`. Return false if EOF has been encountered.
    static bool getLine(bsl::string* out);

    /// Parse the specified `input` which is assumed to be a verb and a
    /// sequence of key/value pairs, and load the specified `verb` with the
    /// first word and the specified `output` with a JSON object containing
    /// the remaining of `input`. The key/value pairs in `input` are assumed
    /// to be separated with spaces or commas, and the key and value in a
    /// given pair are assumed to be separated by either "=" or ":".
    /// For example, if input' = "post payload=\"abc\" version=1.0"
    ///            then `verb` = `post`
    ///            and `output` = "{\"payload\":\"abc\", \"version\":1.0}
    static void preprocessInput(bsl::string*                     verb,
                                bsl::string*                     output,
                                const bsl::string&               input,
                                bsl::unordered_set<bsl::string>* keys = 0);
    static void
    populateProperties(bmqa::MessageProperties*            out,
                       const bsl::vector<MessageProperty>& properties);

    static void
    verifyProperties(const bmqa::MessageProperties&      in,
                     const bsl::vector<MessageProperty>& properties);

    static bool populateSubscriptions(bmqt::QueueOptions*              out,
                                      const bsl::vector<Subscription>& in);

    template <typename CMD>
    static bool parseCommand(CMD*               command,
                             bsl::string*       error,
                             const bsl::string& jsonInput);

    /// Decode hexdump produced by bdlb::Print::hexDump() into binary format.
    /// Read hexdump from the specified `in` until empty line and write binary
    /// into the specified `out`. Return true on success and false on error in
    /// which case load the error description into the optionally specified
    /// `error`.
    static bool decodeHexDump(bsl::ostream*     out,
                              bsl::ostream*     error,
                              bsl::istream&     in,
                              bslma::Allocator* allocator);

    /// Load message content from the specified `filePath` (created by
    /// QueueEngineUtil::dumpMessageInTempfile() method) into
    /// the specified `payload` and `properties`. Return true on success and
    /// false on error in which case load the error description into the
    /// optionally specified `error`.
    static bool loadMessageFromFile(bsl::ostream*      payload,
                                    bsl::ostream*      properties,
                                    bsl::ostream*      error,
                                    const bsl::string& filePath,
                                    bslma::Allocator*  allocator);

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQTOOL.INPUTUTIL");
};

/// Decode the specified `jsonInput` into the specified `command` object
/// of type `CMD`.  Return true on success and false on error in which case
/// load the error description into the specified `error`.
template <typename CMD>
bool InputUtil::parseCommand(CMD*               command,
                             bsl::string*       error,
                             const bsl::string& jsonInput)
{
    bsl::istringstream     is(jsonInput);
    baljsn::DecoderOptions options;
    options.setSkipUnknownElements(false);
    baljsn::Decoder decoder;
    int             rc = decoder.decode(is, command, options);

    if (rc != 0) {
        *error = decoder.loggedMessages();
        return false;  // RETURN
    }

    return true;
}

}  // close package namespace
}  // close enterprise namespace

#endif
