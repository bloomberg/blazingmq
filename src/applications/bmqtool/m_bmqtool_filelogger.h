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

// m_bmqtool_filelogger.h                                             -*-C++-*-
#ifndef INCLUDED_M_BMQTOOL_FILELOGGER
#define INCLUDED_M_BMQTOOL_FILELOGGER

//@PURPOSE: Provide a mechanism to log BlazingMQ events to a file.
//
//@DESCRIPTION: This component writes events and associated details to a
// target log file.  The generic log format is as follow:
//   Timestamp | EventType | EventDetails
// where:
///: o Timestamp is the time of the event, in UTC
///: o EventType (string) either corresponds to one of the
///:   'bmqt::SessionEventType' if the record concerns a session event, or one
///:   of the 'PUT', 'ACK', 'CONFIRM', 'PUSH' values if this is related to a
///:   message record.
///: o EventDetails is 'EventType' specific, refer to the examples below for
///:   the specifics of the format.
//
// Note that every write operation is a no-op if the 'FileLogger' is disabled
// (i.e. 'open' has not been called, or failed).
//
/// Format Examples
///---------------
// Session Event (e.g. CONNECTED, OPEN_QUEUE_RESULT):
//..
//  Timestamp Pid:ThreadId Severity SESSION SessionEventType|QueueUri
//  (QueueUri is optional, if related to a queueOp)
//  2020-02-12T21:20:20.594Z 63884:140678484993792 INFO SESSION CONNECTED|
//..
//
// ACK Message:
//..
//  Timestamp Pid:ThreadId Severity ACK QueueUri|CorrelationId|GUID|AckStatus
//  2020-02-12T21:20:24.694Z 63889:140111938766592 INFO ACK bmq://domain/queue|
//  [ autoValue = 8 ]|0000830000C245BB2FB50C2519157B06|0|
//..
//
// CONFIRM Message:
//..
//  Timestamp Pid:ThreadId Severity CONFIRM QueueUri|MessageGUID
//  2020-02-12T21:20:24.690Z 63884:140678484993792 INFO CONFIRM bmq://domain/qu
//  eue|0000820000C245BAF40A702519157B06|
//..
//
// PUSH Message:
//..
//  Timestamp Pid:ThreadId Severity PUSH QueueURI|MessageGUID| Payload
//  2020-02-12T21:20:24.689Z 63884:140678484993792 INFO PUSH bmq://domain/queue
//  |0000820000C245BAF40A702519157B06|payload
//..
//
// PUT Message:
//..
//  Timestamp Pid:ThreadId Severity PUT QueueUri|CorrelationId|Payload
//  2020-02-12T21:20:24.684Z 63889:140111930373888 INFO PUT bmq://domain/queue|
//  [ autoValue = 8 ]|payload
//..
//

// BMQ
#include <bmqa_message.h>
#include <bmqa_sessionevent.h>

// BDE
#include <ball_fileobserver2.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace m_bmqtool {

// ================
// class FileLogger
// ================

/// Mechanism to log BlazingMQ events to a file.
class FileLogger {
  private:
    bsl::string         d_filePath;  // Path to the file where to log to
    ball::FileObserver2 d_out;       // File Stream to use

  private:
    // NOT IMPLEMENTED
    FileLogger(const FileLogger&);
    FileLogger& operator=(const FileLogger&);

  public:
    // CREATORS

    /// Initialize the FileLogger with the specified `filePath`.
    FileLogger(const bsl::string& filePath, bslma::Allocator* allocator);

    /// Destructor.
    ~FileLogger();

    // MANIPULATORS
    bool open();
    // Open the file and return 'true' on success, 'false' otherwise.

    void close();
    // Close the file.

    void writeSessionEvent(const bmqa::SessionEvent& event);
    void writeAckMessage(const bmqa::Message& message);
    void writeConfirmMessage(const bmqa::Message& message);
    void writePushMessage(const bmqa::Message& message);

    /// Write a line to the file representing the corresponding action in
    /// the specified `event` or `message.  Note that if `open' was not
    /// called on this object, all those methods are no-op.
    void writePutMessage(const bmqa::Message& message);

    // ACCESSORS

    /// Return true if this object is logging to a file, and false
    /// otherwise.  The `FileLogger` will log to a file if `open` was
    /// successfully called, and the file was not yet closed by a call to
    /// `close`.
    bool isOpen() const;

    /// Return the path to the file this object is logging to.
    const bsl::string& filePath() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// class FileLogger
// ----------------

inline bool FileLogger::isOpen() const
{
    return d_out.isFileLoggingEnabled();
}

inline const bsl::string& FileLogger::filePath() const
{
    return d_filePath;
}

}  // close package namespace
}  // close enterprise namespace

#endif
