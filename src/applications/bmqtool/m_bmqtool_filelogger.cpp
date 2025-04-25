// Copyright 2016-2023 Bloomberg Finance L.P.
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

// m_bmqtool_filelogger.cpp                                           -*-C++-*-
#include <m_bmqtool_filelogger.h>

// BMQ
#include <bmqa_queueid.h>
#include <bmqt_correlationid.h>
#include <bmqt_messageguid.h>
#include <bmqt_sessioneventtype.h>
#include <bmqt_uri.h>

// BMQ
#include <bmqu_blob.h>

// BDE
#include <ball_context.h>
#include <ball_recordstringformatter.h>
#include <bdlb_print.h>
#include <bdlbb_blob.h>
#include <bdls_processutil.h>
#include <bdlt_currenttime.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace m_bmqtool {

namespace {

const char k_LOG_SESSION_CATEGORY[] = "SESSION";
const char k_LOG_PUSH_CATEGORY[]    = "PUSH";
const char k_LOG_ACK_CATEGORY[]     = "ACK";
const char k_LOG_PUT_CATEGORY[]     = "PUT";
const char k_LOG_CONFIRM_CATEGORY[] = "CONFIRM";

class LogRecord {
  private:
    ball::Record         d_record;
    ball::FileObserver2& d_out;
    bsl::ostream         d_os;

  public:
    LogRecord(ball::FileObserver2& out,
              const char*          category,
              int                  severity = ball::Severity::e_INFO)
    : d_record()
    , d_out(out)
    , d_os(init(d_record, category, severity))
    {
    }

    ~LogRecord()
    {
        d_out.publish(
            d_record,
            ball::Context(ball::Transmission::e_MANUAL_PUBLISH, 0, 1));
    }

    template <typename T>
    friend LogRecord& operator<<(LogRecord& record, const T& value);

  private:
    static bdlsb::MemOutStreamBuf*
    init(ball::Record& record, const char* category, int severity)
    {
        ball::RecordAttributes& attributes = record.fixedFields();

        attributes.setTimestamp(bdlt::CurrentTime::utc());
        attributes.setProcessID(bdls::ProcessUtil::getProcessId());
        attributes.setThreadID(bslmt::ThreadUtil::selfIdAsUint64());
        attributes.setCategory(category);
        attributes.setSeverity(severity);
        // attributes.setFileName(__FILE__);
        // attributes.setLineNumber(__LINE__);

        attributes.clearMessage();
        return &attributes.messageStreamBuf();
    }
};

template <typename T>
LogRecord& operator<<(LogRecord& record, const T& value)
{
    record.d_os << value;
    return record;
}
static void printSingleLine(LogRecord& record, const bdlbb::Blob& blob)
{
    for (int i = 0; i < blob.numDataBuffers(); ++i) {
        record << bslstl::StringRef(blob.buffer(i).data(),
                                    bmqu::BlobUtil::bufferSize(blob, i));
    }
}

};  // close unnamed namespace

// ================
// class FileLogger
// ================

FileLogger::FileLogger(const bsl::string& filePath,
                       bslma::Allocator*  allocator)
: d_filePath(filePath)
, d_out(allocator)
{
    // NOTHING
}

FileLogger::~FileLogger()
{
    close();
}

bool FileLogger::open()
{
    d_out.enableFileLogging(d_filePath.c_str());

    d_out.setLogFileFunctor(
        ball::RecordStringFormatter("\n%I %p:%t %s %c %m %u"));
    return d_out.isFileLoggingEnabled();
}

void FileLogger::close()
{
    d_out.disableFileLogging();
}

void FileLogger::writeSessionEvent(const bmqa::SessionEvent& event)
{
    if (!isOpen()) {
        // FileLogging is disabled.
        return;  // RETURN
    }

    LogRecord record(d_out, k_LOG_SESSION_CATEGORY);
    record << event.type() << "|";  // session event type

    if (event.type() == bmqt::SessionEventType::e_QUEUE_OPEN_RESULT ||
        event.type() == bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT ||
        event.type() == bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT) {
        record << event.queueId().uri().asString();  // queueUri
    }
}

void FileLogger::writeAckMessage(const bmqa::Message& message)
{
    if (!isOpen()) {
        // FileLogging is disabled.
        return;  // RETURN
    }

    LogRecord record(d_out, k_LOG_ACK_CATEGORY);
    record << message.queueId().uri().asString() << "|"  // QueueUri
           << message.correlationId() << "|"             // CorrelationId
           << message.messageGUID() << "|"               // MessageGUID
           << message.ackStatus() << "|";                // AckStatus
}

void FileLogger::writeConfirmMessage(const bmqa::Message& message)
{
    if (!isOpen()) {
        // FileLogging is disabled.
        return;  // RETURN
    }

    LogRecord record(d_out, k_LOG_CONFIRM_CATEGORY);
    record << message.queueId().uri().asString() << "|"  // QueueUri
           << message.messageGUID() << "|";              // MessageGUID
}

void FileLogger::writePushMessage(const bmqa::Message& message)
{
    if (!isOpen()) {
        // FileLogging is disabled.
        return;  // RETURN
    }

    LogRecord record(d_out, k_LOG_PUSH_CATEGORY);
    record << message.queueId().uri().asString() << "|"  // QueueUri
           << message.messageGUID() << "|";              // MessageGUID

    bdlbb::Blob payload;
    message.getData(&payload);
    printSingleLine(record, payload);  // Payload
}

void FileLogger::writePutMessage(const bmqa::Message& message)
{
    if (!isOpen()) {
        // FileLogging is disabled.
        return;  // RETURN
    }

    LogRecord record(d_out, k_LOG_PUT_CATEGORY);
    record << message.queueId().uri().asString() << "|"  // QueueUri
           << message.correlationId() << "|"             // CorrelationId
           << message.messageGUID() << "|";              // MessageGUID

    bdlbb::Blob payload;
    message.getData(&payload);
    printSingleLine(record, payload);  // Payload
}

}  // close package namespace
}  // close enterprise namespace
