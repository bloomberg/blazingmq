// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqtsk_logcontroller.h                                             -*-C++-*-
#ifndef INCLUDED_BMQTSK_LOGCONTROLLER
#define INCLUDED_BMQTSK_LOGCONTROLLER

//@PURPOSE: Provide a mechanism to interface with the ball log infrastructure.
//
//@CLASSES:
//  bmqtsk::LogController:       mechanism to interface with the ball system
//  bmqtsk::LogControllerConfig: VST for the configuration of a 'LogController'
//
//@DESCRIPTION: 'bmqtsk::LogController' is a mechanism to initialize and
// manipulate the ball log infrastructure.  'bmqtsk::LogControllerConfig' is a
// value-semantic type object used to provide configuration parameters to an
// 'bmqtsk::LogController'.  'LogController' uses a 'ball::AsyncFileObserver'
// to asynchronously write logs to a file, and 'bmqtsk::ConsoleObserver' to
// write logs to stdout.  It offers M-Trap like command processing mechanism to
// dynamically interact with the logging facility (change the verbosity level,
// the console output severity threshold, or change severity level on a
// specific category).
//
// Typical usage of this component is to create it as early as possible, in
// main, and keep it until the end of the application.
//
/// LogControllerConfig: Datum format
///---------------------------------
// The LogControllerConfig can be populated from a datum that must conform with
// the following schema:
//..
//  <complexType name='SyslogConfig'>
//    <sequence>
//      <element name='enabled'   type='boolean' default='false'/>
//      <element name='appName'   type='string'/>
//      <element name='logFormat' type='string'/>
//      <element name='verbosity' type='string'/>
//    </sequence>
//  </complexType>
//
//  <complexType name='LogController'>
//    <sequence>
//      <element name='fileName'                 type='string'/>
//      <element name='fileMaxAgeDays'           type='int'/>
//      <element name='rotationBytes'            type='int'/>
//      <element name='logfileFormat'            type='string'/>
//      <element name='consoleFormat'            type='string'/>
//      <element name='loggingVerbosity'         type='string'/>
//      <element name='bslsLogSeverityThreshold' type='string'/>
//      <element name='consoleSeverityThreshold' type='string'/>
//      <element name='categories'               type='string'
//                                               maxOccurs='unbounded'/>
//        <!-- format for 'categories': 'categoryExpression:severity:color' -->
//      <element name='syslog'                   type='tns:SyslogConfig'/>
//    </sequence>
//  </complexType>
//..
//
/// Logs cleanup
///------------
// LogController will use an 'bmqtsk::LogCleaner' to periodically delete old
// logs: if the 'fileMaxAgeDays' property from the 'LogControllerConfig' is non
// 0, all logs older than this parameter will be deleted at startup, as well as
// every 24 hours.
//
/// Last log symlink
///----------------
// A symbolic name is created in the same directory as the logs to point to the
// last log file, updated upon log rotation.  The symlink name is the derived
// base name of the log file pattern, suffixed with '.last'.
//
/// Usage Example
///-------------
// The following example illustrates how to typically instantiate and use a
// 'LogController'.
//
// First, we create a 'LogControllerConfig' object and configure it with some
// appropriate values.  Note that the default values are good, so we just set
// the log filename.
//..
//  bmqtsk::LogControllerConfig config(allocator);
//
//  config.setFileName("/tmp/mytask.log.%T");
//..
//
// We can then create a 'LogController', and initialize it with the config.
//..
//  int                rc ;
//  bsl::ostringstream errorDescription;
//
//  bmqtsk::LogController logController(allocator);
//  rc = logController.initialize(errorDescription, config);
//  if (rc != 0) {
//    bsl::cerr << "Failed to initialize logController [rc: " << rc
//              << ", error: '" << errorDescription.str() << "']" << bsl::endl;
//    return -1;                                                      // RETURN
//  }
//..
//
// The ball logging system is now initialized, and ball functionality such as
// the 'BALL_LOG_xxx' macros can be used.  The 'LoggerController' manipulators
// can also be used anytime to dynamically configure the logging.
//
// Finally, during shutdown, we can teardown the log controller.
//..
//  logController.shutdown();
//..
//

#include <bmqtsk_alarmlog.h>
#include <bmqtsk_consoleobserver.h>
#include <bmqtsk_logcleaner.h>
#include <bmqtsk_syslogobserver.h>

// BDE
#include <ball_asyncfileobserver.h>
#include <ball_multiplexobserver.h>
#include <ball_severity.h>
#include <ball_severityutil.h>
#include <bsl_map.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdld {
class Datum;
}
namespace bdlmt {
class EventScheduler;
}

namespace bmqtsk {

// =========================
// class LogControllerConfig
// =========================

/// VST representing the configuration for a `LogController` object.
class LogControllerConfig {
  public:
    // TYPES

    /// Small struct holding properties associated to a log category.
    struct CategoryProperties {
        // PUBLIC DATA
        ball::Severity::Level d_verbosity;  // The severity property
        bsl::string           d_color;      // The color property

        // CREATORS

        /// Create a new object having the optionally specified `verbosity`
        /// and `color` properties.
        explicit CategoryProperties(
            ball::Severity::Level verbosity = ball::Severity::e_OFF,
            const bsl::string&    color     = "");
    };

    /// A map of category name to category properties
    /// NB: we *must* use a lexicographically ordered `map`, not an
    /// `unordered_map`, because we want the parent categories to appear
    /// before their child categories when iterating.  This is important
    /// when we register the categories with BALL.  If category is followed
    /// by a parent, the parent's level will prevail.
    typedef bsl::map<bsl::string, CategoryProperties> CategoryPropertiesMap;

  private:
    // DATA
    bsl::string d_fileName;
    // Name of the file to use for logging

    int d_fileMaxAgeDays;
    // Maximum age, in days, to keep log files.

    int d_rotationBytes;
    // Size (in bytes) at which to rotate the log file

    int d_rotationSeconds;
    // Time (in seconds) at which to rotate the log
    // file

    bsl::string d_logfileFormat;
    // Format of log records (for the log file)

    bsl::string d_consoleFormat;
    // Format of log records (for the console output)

    ball::Severity::Level d_loggingVerbosity;
    // Verbosity of logging

    bsls::LogSeverity::Enum d_bslsLogSeverityThreshold;
    // Severity threshold for entire bsls Logger

    ball::Severity::Level d_consoleSeverityThreshold;
    // Severity threshold determining which log records
    // are printed to the Console

    bool d_syslogEnabled;
    // Enable logging to a system log.

    bsl::string d_syslogFormat;
    // Format of log records (for the syslog output)

    bsl::string d_syslogAppName;
    // Application name to use for a syslog name

    ball::Severity::Level d_syslogVerbosity;
    // Verbosity of logging to the syslog

    CategoryPropertiesMap d_categories;
    // Map of category properties

    int d_recordBufferSizeBytes;
    // Size in bytes of the logger's record buffer

    ball::Severity::Level d_recordingVerbosity;
    // If the severity level of the record is at least as severe as the
    // d_recordingVerbosity, then the record will be stored by the logger in
    // its log record buffer (i.e., it will be recorded).

    ball::Severity::Level d_triggerVerbosity;
    // If the severity of the record is at least as severe as the
    // d_triggerVerbosity, then the record will cause immediate publication
    // of that record and any records in the logger's log record buffer (i.e.,
    // this record will trigger a log record dump).

  private:
    /// Convert specified BALL severity `level` to the corresponding
    /// BSLS_LOG severity level.
    static bsls::LogSeverity::Enum
    balToBslsLogLevel(ball::Severity::Level level);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(LogControllerConfig,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `LogControllerConfig` using the optionally specified
    /// `allocator`.  If `allocator` is 0, the currently installed default
    /// allocator is used.
    explicit LogControllerConfig(bslma::Allocator* allocator = 0);

    /// Copy construct a new object from the specified `other`, using the
    /// optionally specified `allocator`.
    LogControllerConfig(const LogControllerConfig& other,
                        bslma::Allocator*          allocator = 0);

    // MANIPULATORS

    /// Assignment operator
    LogControllerConfig& operator=(const LogControllerConfig& rhs);

    LogControllerConfig& setFileName(const bslstl::StringRef& value);
    LogControllerConfig& setFileMaxAgeDays(int value);
    LogControllerConfig& setRotationBytes(int value);
    LogControllerConfig& setRotationSeconds(int value);
    LogControllerConfig& setLogfileFormat(const bslstl::StringRef& value);
    LogControllerConfig& setConsoleFormat(const bslstl::StringRef& value);
    LogControllerConfig& setLoggingVerbosity(ball::Severity::Level value);
    LogControllerConfig&
    setBslsLogSeverityThreshold(bsls::LogSeverity::Enum value);
    LogControllerConfig&
    setConsoleSeverityThreshold(ball::Severity::Level value);
    LogControllerConfig& setSyslogEnabled(bool value);
    LogControllerConfig& setSyslogFormat(const bslstl::StringRef& value);
    LogControllerConfig& setSyslogAppName(const bslstl::StringRef& value);
    LogControllerConfig& setRecordBufferSize(int value);

    /// Set the corresponding attribute to the specified `value` and return
    /// a reference offering modifiable access to this object.
    LogControllerConfig& setSyslogVerbosity(ball::Severity::Level value);

    /// Add the definition from the specified `properties` (format:
    /// "categoryExpression:verbosity:color") to the category properties
    /// list.  Return 0 on success, or a non-zero error code if `category`
    /// is wrongly formatted.
    int addCategoryProperties(const bsl::string& properties);

    /// Clear the registered list of category properties.
    void clearCategoriesProperties();

    /// Populate members of this object from the corresponding fields in the
    /// specified `obj` (which should be of a type compatible with one
    /// generated from a schema as described at the top in the component
    /// level documentation).  Return 0 on success, or a non-zero return
    /// code on error, populating the specified `errorDescription` with a
    /// description of the error.
    template <typename OBJ>
    int fromObj(bsl::ostream& errorDescription, const OBJ& obj);

    // ACCESSORS
    const bsl::string&      fileName() const;
    int                     fileMaxAgeDays() const;
    int                     rotationBytes() const;
    int                     rotationSeconds() const;
    const bsl::string&      logfileFormat() const;
    const bsl::string&      consoleFormat() const;
    ball::Severity::Level   loggingVerbosity() const;
    bsls::LogSeverity::Enum bslsLogSeverityThreshold() const;
    ball::Severity::Level   consoleSeverityThreshold() const;
    bool                    syslogEnabled() const;
    const bsl::string&      syslogFormat() const;
    const bsl::string&      syslogAppName() const;
    ball::Severity::Level   syslogVerbosity() const;
    int                     recordBufferSizeBytes() const;
    ball::Severity::Level   recordingVerbosity() const;
    ball::Severity::Level   triggerVerbosity() const;

    /// Return the value of the corresponding attribute.
    const CategoryPropertiesMap& categoriesProperties() const;
};

// ===================
// class LogController
// ===================

/// Mechanism to initialize and manipulate the ball log infrastructure.
class LogController {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use, held nt owned

    bool d_isInitialized;
    // True if this component has been fully
    // initialized

    LogControllerConfig d_config;
    // Configuration in use by this object.

    ball::MultiplexObserver d_multiplexObserver;
    // The Multiplex observer, registered as
    // the singleton ball::LoggerManager.

    ball::AsyncFileObserver d_fileObserver;
    // Observer for logging to a file.

    AlarmLog d_alarmLog;
    // Log observer to emit alarms to stderr.

    ConsoleObserver d_consoleObserver;
    // Observer for printing to stdout.

    SyslogObserver d_syslogObserver;
    // Observer for printing to system log.

    LogCleaner d_logCleaner;
    // Mechanism to clean up old logs.

    bsl::string d_lastLogLinkPath;
    // The path of the symbolic link pointing to
    // the last log entry.

    bsl::vector<ball::Observer*> d_registeredObservers;
    // Observers successfully registered in a
    // logging system.

  private:
    // PRIVATE MANIPULATORS

    /// Callack invoked by BALL after the log file has been rotated, with
    /// the specified `status` indicating the result of the operation, and
    /// the specified `rotatedFileName` the name of the old log file.
    void afterFileRotation(int status, const bsl::string& rotatedFileName);

    /// Create or update the symbolic link to the latest log file.
    void updateLastLogSymlink();

    /// Process the `info` command from the specified `cmd`, and write the
    /// response in the specified `os`.  Return 0 on success, or a non-zero
    /// error code on failure, and write to `os` a description of the error.
    /// This command doesn't take any extra argument and simply prints some
    /// information about the current setup of the logging, such as the
    /// verbosity, category information, ...
    int processInfoCommand(bsl::istream& cmd, bsl::ostream& os);

    /// Process the `verb` command from the specified `cmd`, and write the
    /// response in the specified `os`.  Return 0 on success, or a non-zero
    /// error code on failure, and write to `os` a description of the error.
    /// This command takes one parameter, the verbosity, and changes the
    /// logging verbosity to the specified one.
    int processVerbCommand(bsl::istream& cmd, bsl::ostream& os);

    /// Process the `console` command from the specified `cmd`, and write
    /// the response in the specified `os`.  Return 0 on success, or a
    /// non-zero error code on failure, and write to `os` a description of
    /// the error.  This command takes one parameter, the severity
    /// threshold, and changes the console severity threshold to the
    /// specified one.
    int processConsoleCommand(bsl::istream& cmd, bsl::ostream& os);

    /// Process the `category` command from the specified `cmd`, and write
    /// the response in the specified `os`.  Return 0 on success, or a
    /// non-zero error code on failure, and write to `os` a description of
    /// the error.  This command takes 3 parameters, the category
    /// expression, the severity and an optional color and will change the
    /// logging severity threshold of the specified category to the
    /// specified verbosity.  If an optional color is provided, it will also
    /// register that category with the color to the console observer,
    /// otherwise it will clear any previously associated color.
    int processCategoryCommand(bsl::istream& cmd, bsl::ostream& os);

    /// Try to register the specified `observer` for logging.  In case of a
    /// failure deregister all previously registered observers for clear
    /// shutdown of a logging system.  Return 0 on success, or a non-zero
    /// error code on a failure.
    int tryRegisterObserver(ball::Observer* observer);

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    LogController(const LogController&);             // = delete
    LogController& operator=(const LogController&);  // = delete

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(LogController, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object using the specified `scheduler` and `allocator`.
    LogController(bdlmt::EventScheduler* scheduler,
                  bslma::Allocator*      allocator);

    /// Destroy this object.  If the LogController was successfully
    /// initialized, `shutdown()` must be called prior to destruction of
    /// this object.
    ~LogController();

    // MANIPULATORS

    /// Initialize this component and the ball logging system, using the
    /// specified `config`.  Return 0 on success, or a non-zero return code
    /// and populates the specified `errorDescription` with a description of
    /// the error on failure.  It is undefined behavior to call this method
    /// more than once.  Once this method return, ball has been enabled and
    /// can be used.
    int initialize(bsl::ostream&              errorDescription,
                   const LogControllerConfig& config);

    /// Stops and shutdown all observers and ball related mechanisms.  This
    /// must be called before destroying this object, to ensure proper
    /// controlled shutdown sequence.  Once this method return, ball system
    /// will no longer be available.
    void shutdown();

    /// Change the logging severity threshold to the specified verbosity
    /// levels: any record with a severity of at least `passVerbosity` will
    /// be printed immediately to the log file, and eventually to the
    /// console if the configured console severity threshold allows it.
    /// any record with a severity of at least `recordingVerbosity` will be
    /// stored by the logger in its log record buffer. Any record with a
    /// severity of at least `triggerVerbosity` will cause immediate
    /// publication of that record and any records in the logger's buffer.
    void setVerbosityLevel(
        ball::Severity::Level passVerbosity,
        ball::Severity::Level recordVerbosity  = ball::Severity::e_OFF,
        ball::Severity::Level triggerVerbosity = ball::Severity::e_OFF);

    /// Change the verbosity of the specified `category` to the specified
    /// `verbosity`.  `category` can be an expression, with a terminating
    /// `*`.
    void setCategoryVerbosity(const bsl::string&    category,
                              ball::Severity::Level verbosity);

    /// Process the command from the specified `cmd`, and write the response
    /// in the specified `os`.  Return 0 on success, or a non-zero error
    /// code on failure, and write to `os` a description of the error.
    int processCommand(const bsl::string& cmd, bsl::ostream& os);

    /// Return a reference offering modifiable access to the console
    /// observer.
    ConsoleObserver& consoleObserver();

    /// Return a reference offering modifiable access to the async file
    /// observer.
    ball::AsyncFileObserver& fileObserver();

    // ACCESSORS

    /// Return a reference not offering modifiable access to the
    /// configuration used by this object.
    const LogControllerConfig& config() const;

    /// Return a reference not offering modifiable access to the console
    /// observer.
    const ConsoleObserver& consoleObserver() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class LogControllerConfig
// -------------------------

template <typename OBJ>
int LogControllerConfig::fromObj(bsl::ostream& errorDescription,
                                 const OBJ&    obj)
{
    (*this)
        .setFileName(obj.fileName())
        .setFileMaxAgeDays(obj.fileMaxAgeDays())
        .setRotationBytes(obj.rotationBytes())
        .setLogfileFormat(obj.logfileFormat())
        .setConsoleFormat(obj.consoleFormat())
        .setSyslogEnabled(obj.syslog().enabled())
        .setSyslogAppName(obj.syslog().appName())
        .setSyslogFormat(obj.syslog().logFormat())
        .setRecordBufferSize(32768);

    if (ball::SeverityUtil::fromAsciiCaseless(
            &d_loggingVerbosity,
            obj.loggingVerbosity().c_str()) != 0) {
        errorDescription << "Invalid value for 'loggingVerbosity' ('"
                         << obj.loggingVerbosity() << "')";
        return -1;  // RETURN
    }

    d_recordingVerbosity = ball::Severity::e_OFF;
    d_triggerVerbosity   = ball::Severity::e_OFF;

    ball::Severity::Level bslsSeverityAsBal = ball::Severity::e_ERROR;
    // TODO: enforcing 'obj' to have 'bslsLogSeverityThreshold' accessor is a
    // backward incompatible change from build perspective, and will require a
    // bulk promotion of dependents, which can be tackled later.  For now, we
    // just assume a log severity level of 'ERROR'.
    // if (ball::SeverityUtil::fromAsciiCaseless(
    //                             &bslsSeverityAsBal,
    //                             obj.bslsLogSeverityThreshold().c_str()) !=
    //                             0) {
    //     errorDescription << "Invalid value for 'bslsLogSeverityThreshold'
    //     ('"
    //                      << obj.bslsLogSeverityThreshold() << "')";
    //     return -1;                                                    //
    //     RETURN
    // }
    d_bslsLogSeverityThreshold = LogControllerConfig::balToBslsLogLevel(
        bslsSeverityAsBal);

    if (ball::SeverityUtil::fromAsciiCaseless(
            &d_consoleSeverityThreshold,
            obj.consoleSeverityThreshold().c_str()) != 0) {
        errorDescription << "Invalid value for 'consoleSeverityThreshold' ('"
                         << obj.consoleSeverityThreshold() << "')";
        return -1;  // RETURN
    }

    if (syslogEnabled() && ball::SeverityUtil::fromAsciiCaseless(
                               &d_syslogVerbosity,
                               obj.syslog().verbosity().c_str()) != 0) {
        errorDescription << "Invalid value for 'syslogVerbosity' ('"
                         << obj.syslog().verbosity() << "')";
        return -1;  // RETURN
    }

    for (bsl::vector<bsl::string>::const_iterator it =
             obj.categories().begin();
         it != obj.categories().end();
         ++it) {
        const int rc = addCategoryProperties(*it);
        if (rc != 0) {
            errorDescription << "Invalid string format for categories entry '"
                             << *it << "' (rc: " << rc << ")";
            return -2;  // RETURN
        }
    }

    return 0;
}

inline LogControllerConfig&
LogControllerConfig::setFileName(const bslstl::StringRef& value)
{
    d_fileName = value;
    return *this;
}

inline LogControllerConfig& LogControllerConfig::setFileMaxAgeDays(int value)
{
    d_fileMaxAgeDays = value;
    return *this;
}

inline LogControllerConfig& LogControllerConfig::setRotationBytes(int value)
{
    d_rotationBytes = value;
    return *this;
}

inline LogControllerConfig& LogControllerConfig::setRotationSeconds(int value)
{
    d_rotationSeconds = value;
    return *this;
}

inline LogControllerConfig&
LogControllerConfig::setLogfileFormat(const bslstl::StringRef& value)
{
    d_logfileFormat = value;
    return *this;
}

inline LogControllerConfig&
LogControllerConfig::setConsoleFormat(const bslstl::StringRef& value)
{
    d_consoleFormat = value;
    return *this;
}

inline LogControllerConfig&
LogControllerConfig::setLoggingVerbosity(ball::Severity::Level value)
{
    d_loggingVerbosity = value;
    return *this;
}

inline LogControllerConfig&
LogControllerConfig::setBslsLogSeverityThreshold(bsls::LogSeverity::Enum value)
{
    d_bslsLogSeverityThreshold = value;
    return *this;
}

inline LogControllerConfig&
LogControllerConfig::setConsoleSeverityThreshold(ball::Severity::Level value)
{
    d_consoleSeverityThreshold = value;
    return *this;
}

inline LogControllerConfig& LogControllerConfig::setSyslogEnabled(bool value)
{
    d_syslogEnabled = value;
    return *this;
}

inline LogControllerConfig&
LogControllerConfig::setSyslogFormat(const bslstl::StringRef& value)
{
    d_syslogFormat = value;
    return *this;
}

inline LogControllerConfig&
LogControllerConfig::setSyslogAppName(const bslstl::StringRef& value)
{
    d_syslogAppName = value;
    return *this;
}

inline LogControllerConfig& LogControllerConfig::setRecordBufferSize(int value)
{
    d_recordBufferSizeBytes = value;
    return *this;
}

inline LogControllerConfig&
LogControllerConfig::setSyslogVerbosity(ball::Severity::Level value)
{
    d_syslogVerbosity = value;
    return *this;
}

inline const bsl::string& LogControllerConfig::fileName() const
{
    return d_fileName;
}

inline int LogControllerConfig::fileMaxAgeDays() const
{
    return d_fileMaxAgeDays;
}

inline int LogControllerConfig::rotationBytes() const
{
    return d_rotationBytes;
}

inline int LogControllerConfig::rotationSeconds() const
{
    return d_rotationSeconds;
}

inline const bsl::string& LogControllerConfig::logfileFormat() const
{
    return d_logfileFormat;
}

inline const bsl::string& LogControllerConfig::consoleFormat() const
{
    return d_consoleFormat;
}

inline ball::Severity::Level LogControllerConfig::loggingVerbosity() const
{
    return d_loggingVerbosity;
}

inline bsls::LogSeverity::Enum
LogControllerConfig::bslsLogSeverityThreshold() const
{
    return d_bslsLogSeverityThreshold;
}

inline ball::Severity::Level
LogControllerConfig::consoleSeverityThreshold() const
{
    return d_consoleSeverityThreshold;
}

inline bool LogControllerConfig::syslogEnabled() const
{
    return d_syslogEnabled;
}

inline const bsl::string& LogControllerConfig::syslogFormat() const
{
    return d_syslogFormat;
}

inline const bsl::string& LogControllerConfig::syslogAppName() const
{
    return d_syslogAppName;
}

inline ball::Severity::Level LogControllerConfig::syslogVerbosity() const
{
    return d_syslogVerbosity;
}

inline int LogControllerConfig::recordBufferSizeBytes() const
{
    return d_recordBufferSizeBytes;
}

inline ball::Severity::Level LogControllerConfig::recordingVerbosity() const
{
    return d_recordingVerbosity;
}

inline ball::Severity::Level LogControllerConfig::triggerVerbosity() const
{
    return d_triggerVerbosity;
}

inline const LogControllerConfig::CategoryPropertiesMap&
LogControllerConfig::categoriesProperties() const
{
    return d_categories;
}

// -------------------
// class LogController
// -------------------

inline ConsoleObserver& LogController::consoleObserver()
{
    return d_consoleObserver;
}

inline ball::AsyncFileObserver& LogController::fileObserver()
{
    return d_fileObserver;
}

inline const LogControllerConfig& LogController::config() const
{
    return d_config;
}

inline const ConsoleObserver& LogController::consoleObserver() const
{
    return d_consoleObserver;
}

}  // close package namespace
}  // close enterprise namespace

#endif
