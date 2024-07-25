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

// m_bmqtool_parameters.h                                             -*-C++-*-
#ifndef INCLUDED_M_BMQTOOL_PARAMETERS
#define INCLUDED_M_BMQTOOL_PARAMETERS

//@PURPOSE: Provide a class holding command line parameters for 'bmqtool'.
//
//@CLASSES:
//  m_bmqtool::ParametersVerbosity: enum for verbosity mode.
//  m_bmqtool::ParametersMode     : enum for the tool mode.
//  m_bmqtool::ParametersLatency  : enum for latency mode.
//  m_bmqtool::Parameters         : holds all parameter values
//
//@DESCRIPTION: This component provides a value-semantic type holding the
// command-line parameters for the 'bmqtool' program.

// bmqtool
#include <m_bmqtool_messages.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bsls_types.h>

// MQB
#include <mqbs_filestoreprotocol.h>

// MWC
#include <mwcu_stringutil.h>

namespace BloombergLP {

namespace m_bmqtool {

// ==========================
// struct ParametersVerbosity
// ==========================

struct ParametersVerbosity {
    enum Value {
        e_SILENT  // Do not output anything
        ,
        e_TRACE  // Enable TRACE output
        ,
        e_DEBUG  // Enable DEBUG output
        ,
        e_INFO  // Enable INFO output
        ,
        e_WARNING  // Enable WARNING output
        ,
        e_ERROR  // Enable ERROR output
        ,
        e_FATAL  // Enable FATAL output
    };

    // CLASS METHODS
    static bsl::ostream& print(bsl::ostream&              stream,
                               ParametersVerbosity::Value value,
                               int                        level          = 0,
                               int                        spacesPerLevel = 4);

    static const char* toAscii(ParametersVerbosity::Value value);

    static bool fromAscii(ParametersVerbosity::Value* out,
                          const bslstl::StringRef&    str);

    static bool isValid(const bsl::string* str, bsl::ostream& stream);
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream&              stream,
                         ParametersVerbosity::Value value);

// =====================
// struct ParametersMode
// =====================

struct ParametersMode {
    enum Value {
        e_CLI  // Command line mode
        ,
        e_AUTO  // Auto mode
        ,
        e_STORAGE  // Inspect storage
        ,
        e_SYSCHK  // Run in syschk mode
    };

    // CLASS METHODS
    static bsl::ostream& print(bsl::ostream&         stream,
                               ParametersMode::Value value,
                               int                   level          = 0,
                               int                   spacesPerLevel = 4);

    static const char* toAscii(ParametersMode::Value value);

    static bool fromAscii(ParametersMode::Value*   out,
                          const bslstl::StringRef& str);

    static bool isValid(const bsl::string* str, bsl::ostream& stream);
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, ParametersMode::Value value);

// ========================
// struct ParametersLatency
// ========================

struct ParametersLatency {
    enum Value {
        e_NONE  // Do not put any timestamp in the messages
        ,
        e_HIRES  // Use the fast hardware clock
        ,
        e_EPOCH  // Use the system time slower but work accross network
    };

    // CLASS METHODS
    static bsl::ostream& print(bsl::ostream&            stream,
                               ParametersLatency::Value value,
                               int                      level          = 0,
                               int                      spacesPerLevel = 4);

    static const char* toAscii(ParametersLatency::Value value);

    static bool fromAscii(ParametersLatency::Value* out,
                          const bslstl::StringRef&  str);

    static bool isValid(const bsl::string* str, bsl::ostream& stream);
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, ParametersLatency::Value value);

// ================
// class Parameters
// ================

/// Holds command line parameters for bmqtool.
class Parameters {
  private:
    // DATA
    ParametersMode::Value d_mode;
    // Application mode to use
    // Default: CLI

    bsl::string d_broker;
    // Address of the broker
    // Default: tcp://localhost:30114

    bsl::string d_queueUri;
    // QueueUri for auto mode
    // Default: 'bmq://bmq.bench.inMemory/bmqtool'

    bsl::string d_qlistFilePath;
    // Path to read qlist files from
    // Default: ''

    bsl::string d_journalFilePath;
    // Path to read journal files from
    // Default: ''

    bsl::string d_dataFilePath;
    // Path to read data files from
    // Default: ''

    bsl::string d_sequentialMessagePattern;
    // printf format for the message payload
    // content, where a numerical placeholder will
    // be replaced by a monotonically increasing
    // number.

    bsls::Types::Uint64 d_queueFlags;
    // QueueFlags
    // Default: 0

    ParametersLatency::Value d_latency;
    // Latency mode to use
    // Default: NONE

    bsl::string d_latencyReportPath;
    // Path to a file where latency statistics (in
    // JSON) format, will be written.  This
    // parameter only has effect when 'd_latency'
    // is not set to 'NONE'.

    bsl::string d_logFilePath;
    // Path to the file where to log to.

    bool d_dumpMsg;
    // Dump message content

    bool d_confirmMsg;
    // Confirm messages upon reception

    bsl::uint64_t d_eventSize;
    // Number of messages per event
    // Default: 1

    int d_msgSize;
    // Size of the payload of each message (in
    // bytes).  Default: 1024 (min 0, max: XXX)

    int d_postRate;
    // Number of events to publish at every
    // 'd_publishInterval'
    // Default: 1

    int d_postInterval;
    // Interval to publish events (in ms)
    // Default: 1000

    bsl::uint64_t d_eventsCount;
    // if >= 0, number of events to post (in
    // producer mode) before stopping to produce;
    // else infinite
    // Default: 0

    int d_maxUnconfirmedMsgs;
    // Maximum unconfirmed messages, for open
    // queue in 'read' mode.

    int d_maxUnconfirmedBytes;
    // Maximum unconfirmed bytes, for open queue
    // in 'read' mode.

    ParametersVerbosity::Value d_verbosity;
    // Which verbosity level to use
    // Default: INFO

    bsl::string d_logFormat;
    // Which format to use for logging
    // Default: "%d (%t) %s %F:%l %m\n"

    int d_numProcessingThreads;
    // How many threads to create for message
    // handling.

    int d_shutdownGrace;
    // How many seconds to wait before shutting
    // down.

    bool d_noSessionEventHandler;
    // False to use the EventHandler callback,
    // true to use custom event handler threads.

    bool d_memoryDebug;
    // Should we use a testAllocator
    // Default: false

    bsl::vector<MessageProperty> d_messageProperties;

    bsl::vector<Subscription> d_subscriptions;

    bsl::string d_autoIncrementedField;
    // A name of a property to put auto-incremented values
    // in batch-posting mode.

  public:
    // CREATORS

    /// Default constructor
    Parameters(bslma::Allocator* allocator);

    // MANIPULATORS
    Parameters& setMode(ParametersMode::Value value);
    Parameters& setBroker(const bsl::string& value);
    Parameters& setQueueUri(const bsl::string& value);
    Parameters& setQueueFlags(bsls::Types::Uint64 value);
    Parameters& setLatency(ParametersLatency::Value value);
    Parameters& setLatencyReportPath(const bsl::string& value);
    Parameters& setDumpMsg(bool value);
    Parameters& setConfirmMsg(bool value);
    Parameters& setEventSize(bsl::uint64_t value);
    Parameters& setMsgSize(int value);
    Parameters& setPostRate(int value);
    Parameters& setPostInterval(int value);
    Parameters& setEventsCount(bsl::uint64_t value);
    Parameters& setMaxUnconfirmedMsgs(int value);
    Parameters& setMaxUnconfirmedBytes(int value);
    Parameters& setVerbosity(ParametersVerbosity::Value value);
    Parameters& setLogFormat(const bsl::string& value);
    Parameters& setMemoryDebug(bool value);
    Parameters& setNumProcessingThreads(int value);
    Parameters& setNoSessionEventHandler(bool value);
    Parameters& setStoragePath(const bsl::string& value);
    Parameters& setLogFilePath(const bsl::string& value);
    Parameters& setSequentialMessagePattern(const bsl::string& value);
    Parameters& setShutdownGrace(int value);
    Parameters&
    setMessageProperties(const bsl::vector<MessageProperty>& value);
    Parameters& setSubscriptions(const bsl::vector<Subscription>& value);
    Parameters& setAutoIncrementedField(const bsl::string& value);

    // Set the corresponding member to the specified 'value' and return a
    // reference offering modifiable access to this object.

    /// Populate this object by loading the values from the specified
    /// `params` and return true on success, or write a description of the
    /// error in the specified `stream` and return false on failure.
    bool from(bsl::ostream& stream, const CommandLineParameters& params);

    /// Do a nicer pretty print of all the parameters aligned.
    void dump(bsl::ostream& stream);

    /// Validate the consistency of all settings.
    bool validate(bsl::string* error);

    // ACCESSORS

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    ParametersMode::Value               mode() const;
    const bsl::string&                  broker() const;
    const bsl::string&                  queueUri() const;
    const bsl::string&                  journalFilePath() const;
    const bsl::string&                  qlistFilePath() const;
    const bsl::string&                  dataFilePath() const;
    const bsl::string&                  sequentialMessagePattern() const;
    bsls::Types::Uint64                 queueFlags() const;
    ParametersLatency::Value            latency() const;
    const bsl::string&                  latencyReportPath() const;
    const bsl::string&                  logFilePath() const;
    bool                                dumpMsg() const;
    bool                                confirmMsg() const;
    bsl::uint64_t                       eventSize() const;
    int                                 msgSize() const;
    int                                 postRate() const;
    int                                 postInterval() const;
    bsl::uint64_t                       eventsCount() const;
    int                                 maxUnconfirmedMsgs() const;
    int                                 maxUnconfirmedBytes() const;
    ParametersVerbosity::Value          verbosity() const;
    bsl::string                         logFormat() const;
    bool                                memoryDebug() const;
    int                                 numProcessingThreads() const;
    int                                 shutdownGrace() const;
    bool                                noSessionEventHandler() const;
    const bsl::vector<MessageProperty>& messageProperties() const;
    const bsl::vector<Subscription>&    subscriptions() const;
    const bsl::string&                  autoIncrementedField() const;
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, const Parameters& value);

// ============================================================================
//              INLINE AND TEMPLATE FUNCTION IMPLEMENTATIONS
// ============================================================================

// ----------------
// class Parameters
// ----------------

// MANIPULATORS
inline Parameters& Parameters::setMode(ParametersMode::Value value)
{
    d_mode = value;
    return *this;
}

inline Parameters& Parameters::setBroker(const bsl::string& value)
{
    d_broker = value;
    return *this;
}

inline Parameters& Parameters::setQueueUri(const bsl::string& value)
{
    d_queueUri = value;
    return *this;
}

inline Parameters& Parameters::setQueueFlags(bsls::Types::Uint64 value)
{
    d_queueFlags = value;
    return *this;
}

inline Parameters& Parameters::setLatency(ParametersLatency::Value value)
{
    d_latency = value;
    return *this;
}

inline Parameters& Parameters::setLatencyReportPath(const bsl::string& value)
{
    d_latencyReportPath = value;
    return *this;
}

inline Parameters& Parameters::setDumpMsg(bool value)
{
    d_dumpMsg = value;
    return *this;
}

inline Parameters& Parameters::setConfirmMsg(bool value)
{
    d_confirmMsg = value;
    return *this;
}

inline Parameters& Parameters::setEventSize(bsl::uint64_t value)
{
    d_eventSize = value;
    return *this;
}

inline Parameters& Parameters::setMsgSize(int value)
{
    d_msgSize = value;
    return *this;
}

inline Parameters& Parameters::setPostRate(int value)
{
    d_postRate = value;
    return *this;
}

inline Parameters& Parameters::setPostInterval(int value)
{
    d_postInterval = value;
    return *this;
}

inline Parameters& Parameters::setEventsCount(bsl::uint64_t value)
{
    d_eventsCount = value;
    return *this;
}

inline Parameters& Parameters::setMaxUnconfirmedMsgs(int value)
{
    d_maxUnconfirmedMsgs = value;
    return *this;
}

inline Parameters& Parameters::setMaxUnconfirmedBytes(int value)
{
    d_maxUnconfirmedBytes = value;
    return *this;
}

inline Parameters& Parameters::setVerbosity(ParametersVerbosity::Value value)
{
    d_verbosity = value;
    return *this;
}

inline Parameters& Parameters::setLogFormat(const bsl::string& value)
{
    d_logFormat = value;
    return *this;
}

inline Parameters& Parameters::setMemoryDebug(bool value)
{
    d_memoryDebug = value;
    return *this;
}

inline Parameters& Parameters::setNumProcessingThreads(int value)
{
    d_numProcessingThreads = value;
    return *this;
}

inline Parameters& Parameters::setShutdownGrace(int value)
{
    d_shutdownGrace = value;
    return *this;
}

inline Parameters& Parameters::setNoSessionEventHandler(bool value)
{
    d_noSessionEventHandler = value;
    return *this;
}

inline Parameters& Parameters::setStoragePath(const bsl::string& value)
{
    bsl::string dataExt(mqbs::FileStoreProtocol::k_DATA_FILE_EXTENSION);
    bsl::string journalExt(mqbs::FileStoreProtocol::k_JOURNAL_FILE_EXTENSION);
    bsl::string qlistExt(mqbs::FileStoreProtocol::k_QLIST_FILE_EXTENSION);

    // We break up the path into the storage files that
    // are found at the specified file location.
    if (!value.empty()) {
        if (value[value.length() - 1] == '*') {
            // Wild card specifier, so try to match all files.
            bsl::string substr = value.substr(0, value.length() - 1);

            d_qlistFilePath.assign(substr);
            d_qlistFilePath.append(qlistExt);

            d_journalFilePath.assign(substr);
            d_journalFilePath.append(journalExt);

            d_dataFilePath.assign(substr);
            d_dataFilePath.append(dataExt);
        }
        else if (mwcu::StringUtil::endsWith(value, qlistExt)) {
            d_qlistFilePath = value;
        }
        else if (mwcu::StringUtil::endsWith(value, journalExt)) {
            d_journalFilePath = value;
        }
        else if (mwcu::StringUtil::endsWith(value, dataExt)) {
            d_dataFilePath = value;
        }
    }
    return *this;
}

inline Parameters&
Parameters::setSequentialMessagePattern(const bsl::string& value)
{
    d_sequentialMessagePattern = value;
    return *this;
}

inline Parameters& Parameters::setLogFilePath(const bsl::string& value)
{
    d_logFilePath = value;
    return *this;
}

inline Parameters&
Parameters::setMessageProperties(const bsl::vector<MessageProperty>& value)
{
    d_messageProperties = value;
    return *this;
}

inline Parameters&
Parameters::setSubscriptions(const bsl::vector<Subscription>& value)
{
    d_subscriptions = value;
    return *this;
}

inline Parameters&
Parameters::setAutoIncrementedField(const bsl::string& value)
{
    d_autoIncrementedField = value;
    return *this;
}

// ACCESSORS
inline ParametersMode::Value Parameters::mode() const
{
    return d_mode;
}

inline const bsl::string& Parameters::broker() const
{
    return d_broker;
}

inline const bsl::string& Parameters::queueUri() const
{
    return d_queueUri;
}

inline const bsl::string& Parameters::journalFilePath() const
{
    return d_journalFilePath;
}

inline const bsl::string& Parameters::qlistFilePath() const
{
    return d_qlistFilePath;
}

inline const bsl::string& Parameters::dataFilePath() const
{
    return d_dataFilePath;
}

inline const bsl::string& Parameters::sequentialMessagePattern() const
{
    return d_sequentialMessagePattern;
}

inline const bsl::string& Parameters::logFilePath() const
{
    return d_logFilePath;
}

inline bsls::Types::Uint64 Parameters::queueFlags() const
{
    return d_queueFlags;
}

inline ParametersLatency::Value Parameters::latency() const
{
    return d_latency;
}

inline const bsl::string& Parameters::latencyReportPath() const
{
    return d_latencyReportPath;
}

inline bool Parameters::dumpMsg() const
{
    return d_dumpMsg;
}

inline bool Parameters::confirmMsg() const
{
    return d_confirmMsg;
}

inline bsl::uint64_t Parameters::eventSize() const
{
    return d_eventSize;
}

inline int Parameters::msgSize() const
{
    return d_msgSize;
}

inline int Parameters::postRate() const
{
    return d_postRate;
}

inline int Parameters::postInterval() const
{
    return d_postInterval;
}

inline bsl::uint64_t Parameters::eventsCount() const
{
    return d_eventsCount;
}

inline int Parameters::maxUnconfirmedMsgs() const
{
    return d_maxUnconfirmedMsgs;
}

inline int Parameters::maxUnconfirmedBytes() const
{
    return d_maxUnconfirmedBytes;
}

inline ParametersVerbosity::Value Parameters::verbosity() const
{
    return d_verbosity;
}

inline bsl::string Parameters::logFormat() const
{
    return d_logFormat;
}

inline bool Parameters::memoryDebug() const
{
    return d_memoryDebug;
}

inline int Parameters::numProcessingThreads() const
{
    return d_numProcessingThreads;
}

inline int Parameters::shutdownGrace() const
{
    return d_shutdownGrace;
}

inline bool Parameters::noSessionEventHandler() const
{
    return d_noSessionEventHandler;
}

inline const bsl::vector<MessageProperty>&
Parameters::messageProperties() const
{
    return d_messageProperties;
}

inline const bsl::vector<Subscription>& Parameters::subscriptions() const
{
    return d_subscriptions;
}

inline const bsl::string& Parameters::autoIncrementedField() const
{
    return d_autoIncrementedField;
}

}  // close package namespace

// --------------------------
// struct ParametersVerbosity
// --------------------------

inline bsl::ostream&
m_bmqtool::operator<<(bsl::ostream&                         stream,
                      m_bmqtool::ParametersVerbosity::Value value)
{
    return ParametersVerbosity::print(stream, value, 0, -1);
}

// ---------------------
// struct ParametersMode
// ---------------------

inline bsl::ostream&
m_bmqtool::operator<<(bsl::ostream&                    stream,
                      m_bmqtool::ParametersMode::Value value)
{
    return ParametersMode::print(stream, value, 0, -1);
}

// ------------------------
// struct ParametersLatency
// ------------------------

inline bsl::ostream&
m_bmqtool::operator<<(bsl::ostream&                       stream,
                      m_bmqtool::ParametersLatency::Value value)
{
    return ParametersLatency::print(stream, value, 0, -1);
}

// -----------------
// struct Parameters
// -----------------

inline bsl::ostream& m_bmqtool::operator<<(bsl::ostream&                stream,
                                           const m_bmqtool::Parameters& value)
{
    return value.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
