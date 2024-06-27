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

// mwctsk_logcontroller.cpp                                           -*-C++-*-
#include <mwctsk_logcontroller.h>

#include <mwcscm_version.h>
// MWC
#include <mwcu_stringutil.h>

// BDE
#include <ball_administration.h>
#include <ball_log.h>
#include <ball_logfilecleanerutil.h>
#include <ball_loggerfunctorpayloads.h>
#include <ball_loggermanager.h>
#include <ball_loggermanagerconfiguration.h>
#include <bdlb_string.h>
#include <bdld_datum.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_eventscheduler.h>
#include <bdls_filesystemutil.h>
#include <bdls_pathutil.h>
#include <bdlt_datetimeinterval.h>
#include <bdlt_timeunitratio.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_cstring.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_sstream.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

#include <unistd.h>  // symlink()

namespace BloombergLP {
namespace mwctsk {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MWCTSK.LOGCONTROLLER");

void printCategoryWithDelimiter(const bsl::shared_ptr<bsl::ostream>& os,
                                ball::Category*                      category,
                                char                                 delim)
{
    ball::Severity::Level level = static_cast<ball::Severity::Level>(
        category->passLevel());

    *os << "    [" << level << "]" << delim << " ";
    if (bsl::strlen(category->categoryName()) == 0) {
        *os << "*";
    }
    else {
        *os << category->categoryName();
    }
    *os << "\n";
}

static void bslsLogHandler(bsls::LogSeverity::Enum severity,
                           const char*             file,
                           int                     line,
                           const char*             message)
{
    // Custom message handler for bsls::Log.  Redirect the specified
    // 'message', the specified 'file' and the specified 'line' to
    // ball::Log at the BALL severity corresponding to the specified
    // BSLS log 'severity'.
    const ball::Category* category = ball::Log::setCategory("BSLS.LOG");

    ball::Severity::Level balSev;

    switch (severity) {
    case bsls::LogSeverity::e_FATAL: balSev = ball::Severity::e_FATAL; break;
    case bsls::LogSeverity::e_ERROR: balSev = ball::Severity::e_ERROR; break;
    case bsls::LogSeverity::e_WARN: balSev = ball::Severity::e_WARN; break;
    case bsls::LogSeverity::e_INFO: balSev = ball::Severity::e_INFO; break;
    case bsls::LogSeverity::e_DEBUG: balSev = ball::Severity::e_DEBUG; break;
    case bsls::LogSeverity::e_TRACE: balSev = ball::Severity::e_TRACE; break;
    default: balSev = ball::Severity::e_INFO;
    };

    ball::Log::logMessage(category, balSev, file, line, message);
};

}  // close unnamed namespace

// -------------------------
// class LogControllerConfig
// -------------------------

bsls::LogSeverity::Enum
LogControllerConfig::balToBslsLogLevel(ball::Severity::Level level)
{
    switch (level) {
    case ball::Severity::e_FATAL: return bsls::LogSeverity::e_FATAL;
    case ball::Severity::e_ERROR: return bsls::LogSeverity::e_ERROR;
    case ball::Severity::e_WARN: return bsls::LogSeverity::e_WARN;
    case ball::Severity::e_INFO: return bsls::LogSeverity::e_INFO;
    case ball::Severity::e_DEBUG: return bsls::LogSeverity::e_DEBUG;
    case ball::Severity::e_TRACE: return bsls::LogSeverity::e_TRACE;
    default: return bsls::LogSeverity::e_ERROR;
    };
}

LogControllerConfig::CategoryProperties::CategoryProperties(
    ball::Severity::Level verbosity,
    const bsl::string&    color)
: d_verbosity(verbosity)
, d_color(color)
{
    // NOTHING
}

LogControllerConfig::LogControllerConfig(bslma::Allocator* allocator)
: d_fileName(allocator)
, d_fileMaxAgeDays(0)
, d_rotationBytes(128 * 1024 * 1024)  // 128 MB
, d_rotationSeconds(60 * 60 * 24)     // 1 day
, d_logfileFormat("%d (%t) %s %F:%l %m\n\n", allocator)
, d_consoleFormat("%d (%t) %s %F:%l %m\n", allocator)
, d_loggingVerbosity(ball::Severity::INFO)
, d_bslsLogSeverityThreshold(bsls::LogSeverity::e_ERROR)
, d_consoleSeverityThreshold(ball::Severity::OFF)
, d_syslogEnabled(false)
, d_syslogFormat("%d (%t) %s %F:%l %m\n\n", allocator)
, d_syslogAppName("")
, d_syslogVerbosity(ball::Severity::ERROR)
, d_categories(allocator)
{
    // NOTHING
}

LogControllerConfig::LogControllerConfig(const LogControllerConfig& other,
                                         bslma::Allocator*          allocator)
: d_fileName(other.d_fileName, allocator)
, d_fileMaxAgeDays(other.d_fileMaxAgeDays)
, d_rotationBytes(other.d_rotationBytes)
, d_rotationSeconds(other.d_rotationSeconds)
, d_logfileFormat(other.d_logfileFormat, allocator)
, d_consoleFormat(other.d_consoleFormat, allocator)
, d_loggingVerbosity(other.d_loggingVerbosity)
, d_bslsLogSeverityThreshold(other.d_bslsLogSeverityThreshold)
, d_consoleSeverityThreshold(other.d_consoleSeverityThreshold)
, d_syslogEnabled(other.d_syslogEnabled)
, d_syslogFormat(other.d_syslogFormat, allocator)
, d_syslogAppName(other.d_syslogAppName, allocator)
, d_syslogVerbosity(other.d_syslogVerbosity)
, d_categories(other.d_categories, allocator)
{
    // NOTHING
}

LogControllerConfig&
LogControllerConfig::operator=(const LogControllerConfig& rhs)
{
    if (&rhs != this) {
        d_fileName                 = rhs.d_fileName;
        d_fileMaxAgeDays           = rhs.d_fileMaxAgeDays;
        d_rotationBytes            = rhs.d_rotationBytes;
        d_rotationSeconds          = rhs.d_rotationSeconds;
        d_logfileFormat            = rhs.d_logfileFormat;
        d_consoleFormat            = rhs.d_consoleFormat;
        d_loggingVerbosity         = rhs.d_loggingVerbosity;
        d_bslsLogSeverityThreshold = rhs.d_bslsLogSeverityThreshold;
        d_consoleSeverityThreshold = rhs.d_consoleSeverityThreshold;
        d_syslogEnabled            = rhs.d_syslogEnabled;
        d_syslogFormat             = rhs.d_syslogFormat;
        d_syslogAppName            = rhs.d_syslogAppName;
        d_syslogVerbosity          = rhs.d_syslogVerbosity;
        d_categories               = rhs.d_categories;
    }

    return *this;
}

int LogControllerConfig::addCategoryProperties(const bsl::string& properties)
{
    // NOTE:
    //: o the 'properties' parameter *must* remain a string, and not a
    //    stringRef because it's passed to 'mwcu::StringUtil::strTokenizeRef'
    //    which returns references to the 'string' passed as parameter (so we
    //    can't have it perform an implicit stringRef to string conversion
    //    because then the returned stringRef would point to a deleted
    //    temporary).
    //: the format of 'properties' is:  categoryExpression:severity:color

    bsl::vector<bslstl::StringRef> tokens =
        mwcu::StringUtil::strTokenizeRef(properties, ":");

    if (tokens.size() != 3) {
        return -1;  // RETURN
    }

    bsl::string           severityStr(tokens[1].data(), tokens[1].length());
    ball::Severity::Level severity;
    if (ball::SeverityUtil::fromAsciiCaseless(&severity,
                                              severityStr.c_str()) != 0) {
        return -2;  // RETURN
    }

    // Add to categories map
    d_categories[tokens[0]] = CategoryProperties(severity, tokens[2]);
    return 0;
}

void LogControllerConfig::clearCategoriesProperties()
{
    d_categories.clear();
}

int LogControllerConfig::fromDatum(bsl::ostream&      errorDescription,
                                   const bdld::Datum& datum)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS           = 0,
        rc_INVALID_DATUM     = -1,
        rc_INVALID_KEY_TYPE  = -2,
        rc_INVALID_KEY_VALUE = -3,
        rc_UNSET_VALUE       = -4,
        rc_UNKNOWN_KEY       = -5
    };

    if (!datum.isMap()) {
        errorDescription << "The specified datum must be a map";
        return rc_INVALID_DATUM;  // RETURN
    }

#define PARSE_ENTRY(ENTRY, FIELD, TYPE, KEY_STR, KEY_PATH)                    \
    if (bdlb::String::areEqualCaseless(ENTRY.key(), KEY_STR)) {               \
        if (!ENTRY.value().is##TYPE()) {                                      \
            errorDescription << "Key '" << #KEY_PATH << "' type must be a "   \
                             << #TYPE;                                        \
            return rc_INVALID_KEY_TYPE;                                       \
        }                                                                     \
        FIELD = ENTRY.value().the##TYPE();                                    \
        continue;                                                             \
    }

#define PARSE_CONF(FIELD, TYPE, KEY_STR)                                      \
    PARSE_ENTRY(entry, FIELD, TYPE, KEY_STR, KEY_STR)

#define PARSE_SYSLOG(FIELD, TYPE, KEY_STR)                                    \
    PARSE_ENTRY(syslog, FIELD, TYPE, KEY_STR, "syslog/" + KEY_STR)

    double      fileMaxAgeDays = -1;
    double      rotationBytes  = -1;
    bsl::string loggingVerbosityStr;
    bsl::string bslsLogSeverityStr;
    bsl::string consoleSeverityStr;
    bsl::string syslogVerbosityStr;

    // Iterate over each keys of the datum map..
    for (bsl::size_t i = 0; i < datum.theMap().size(); ++i) {
        const bdld::DatumMapEntry& entry = datum.theMap().data()[i];
        PARSE_CONF(d_fileName, String, "fileName");
        PARSE_CONF(fileMaxAgeDays, Double, "fileMaxAgeDays");
        PARSE_CONF(rotationBytes, Double, "rotationBytes");
        PARSE_CONF(d_logfileFormat, String, "logfileFormat");
        PARSE_CONF(d_consoleFormat, String, "consoleFormat");
        PARSE_CONF(loggingVerbosityStr, String, "loggingVerbosity");
        PARSE_CONF(bslsLogSeverityStr, String, "bslsLogSeverityThreshold");
        PARSE_CONF(consoleSeverityStr, String, "consoleSeverityThreshold");

        if (bdlb::String::areEqualCaseless(entry.key(), "categories")) {
            if (!entry.value().isArray()) {
                errorDescription << "Key 'categories' must be an array";
                return rc_INVALID_KEY_TYPE;  // RETURN
            }
            bdld::DatumArrayRef array = entry.value().theArray();
            for (bsls::Types::size_type idx = 0; idx < array.length(); ++idx) {
                if (!array[idx].isString()) {
                    errorDescription << "Invalid type for categories[" << idx
                                     << "]: must be a string";
                    return rc_INVALID_KEY_TYPE;  // RETURN
                }
                int rc = addCategoryProperties(array[idx].theString());
                if (rc != 0) {
                    errorDescription << "Invalid string format for categories"
                                     << "[" << idx << "] [rc: " << rc << "]";
                    return rc_INVALID_KEY_VALUE;  // RETURN
                }
            }
            continue;  // CONTINUE
        }

        if (bdlb::String::areEqualCaseless(entry.key(), "syslog")) {
            if (!entry.value().isMap()) {
                errorDescription << "Key 'syslog' must be a map";
                return rc_INVALID_KEY_TYPE;  // RETURN
            }

            bdld::DatumMapRef syslogConfig = entry.value().theMap();
            for (bsl::size_t j = 0; j < syslogConfig.size(); ++j) {
                const bdld::DatumMapEntry& syslog = syslogConfig.data()[j];

                PARSE_SYSLOG(d_syslogEnabled, Boolean, "enabled");
                PARSE_SYSLOG(d_syslogFormat, String, "logFormat");
                PARSE_SYSLOG(d_syslogAppName, String, "appName");
                PARSE_SYSLOG(syslogVerbosityStr, String, "verbosity");

                // In a normal workflow should just 'continue'
                errorDescription << "Unknown key 'syslog/" << syslog.key()
                                 << "'";
                return rc_UNKNOWN_KEY;  // RETURN
            }

            continue;  // CONTINUE
        }

        // In a normal workflow should just 'continue'
        errorDescription << "Unknown key '" << entry.key() << "'";
        return rc_UNKNOWN_KEY;  // RETURN
    }

#undef PARSE_SYSLOG
#undef PARSE_CONF
#undef PARSE_ENTRY

    if (fileMaxAgeDays <= 0) {
        errorDescription << "Unset key 'fileMaxAgeDays'";
        return rc_UNSET_VALUE;  // RETURN
    }
    else {
        d_fileMaxAgeDays = static_cast<int>(fileMaxAgeDays);
    }

    if (rotationBytes <= 0) {
        errorDescription << "Unset key 'rotationBytes'";
        return rc_UNSET_VALUE;  // RETURN
    }
    else {
        d_rotationBytes = static_cast<int>(rotationBytes);
    }

    if (ball::SeverityUtil::fromAsciiCaseless(&d_loggingVerbosity,
                                              loggingVerbosityStr.c_str()) !=
        0) {
        errorDescription << "Invalid value for 'loggingVerbosity' ('"
                         << loggingVerbosityStr << "')";
        return rc_INVALID_KEY_VALUE;  // RETURN
    }

    ball::Severity::Level bslsSeverityAsBal;
    if (ball::SeverityUtil::fromAsciiCaseless(&bslsSeverityAsBal,
                                              bslsLogSeverityStr.c_str()) !=
        0) {
        errorDescription << "Invalid value for 'bslsLogSeverityThreshold' ('"
                         << bslsLogSeverityStr << "')";
        return rc_INVALID_KEY_VALUE;  // RETURN
    }
    d_bslsLogSeverityThreshold = LogControllerConfig::balToBslsLogLevel(
        bslsSeverityAsBal);

    if (ball::SeverityUtil::fromAsciiCaseless(&d_consoleSeverityThreshold,
                                              consoleSeverityStr.c_str()) !=
        0) {
        errorDescription << "Invalid value for 'consoleSeverityThreshold' ('"
                         << consoleSeverityStr << "')";
        return rc_INVALID_KEY_VALUE;  // RETURN
    }

    if (d_syslogEnabled && ball::SeverityUtil::fromAsciiCaseless(
                               &d_syslogVerbosity,
                               syslogVerbosityStr.c_str()) != 0) {
        errorDescription << "Invalid value for 'syslog/verbosity' ('"
                         << syslogVerbosityStr << "')";
        return rc_INVALID_KEY_VALUE;  // RETURN
    }

    return rc_SUCCESS;
}

// -------------------
// class LogController
// -------------------

void LogController::afterFileRotation(
    BSLS_ANNOTATION_UNUSED int   status,
    BSLS_ANNOTATION_UNUSED const bsl::string& rotatedFileName)
{
    updateLastLogSymlink();
}

void LogController::updateLastLogSymlink()
{
    if (d_lastLogLinkPath.empty()) {
        return;  // RETURN
    }

    bsl::string lastLog;
    if (!d_fileObserver.isFileLoggingEnabled(&lastLog)) {
        return;  // RETURN
    }

    // Ideally we should check existence of the symlink before calling
    // 'remove', but there is no API in BDE to verify that a symlink file exist
    // (regardless of whether it points to a valid file or not).  And anyway,
    // this 'if' check would not provide lots of added value, so just blindly
    // attempt to delete, ignoring potential failure.
    bdls::FilesystemUtil::remove(d_lastLogLinkPath);

    // Since the symlink is created next to the log, in the same directory,
    // ensure to only keep the basename from the 'lastLog' (to prevent wrong
    // symlinking in case the return value was a relative path).
    bsl::string baseName;
    bdls::PathUtil::getBasename(&baseName, lastLog);

    // create symlink
    int rc = symlink(baseName.c_str(), d_lastLogLinkPath.c_str());
    if (rc != 0) {
        BALL_LOG_WARN << "Unable to create lastLog symlink '" << lastLog
                      << "' -> '" << baseName << "'), rc: " << rc;
    }
}

int LogController::processInfoCommand(BSLS_ANNOTATION_UNUSED bsl::istream& cmd,
                                      bsl::ostream&                        os)
{
    // Gather some information
    ball::Severity::Level verbosity = static_cast<ball::Severity::Level>(
        ball::Administration::defaultPassThresholdLevel());

    bsl::string logFile;
    if (!d_fileObserver.isFileLoggingEnabled(&logFile)) {
        logFile = "*disabled*";
    }

    os << "  Logging status:\n"
       << "-----------------\n"
       << "  General:\n"
       << "    Verbosity.................: " << verbosity << "\n"
       << "    BslsLogSeverityThreshold..: " << bsls::Log::severityThreshold()
       << "\n"
       << "    ConsoleSeverityThreshold..: "
       << d_consoleObserver.severityThreshold() << "\n"
       << "  FileObserver:\n"
       << "    LogFile.................: " << logFile << "\n"
       << "    LogRecordQueueLength....: "
       << d_fileObserver.recordQueueLength() << "\n"
       << "  Categories:\n";

    // Iterate over each category to print it's current verbosity
    ball::LoggerManager::singleton().visitCategories(bdlf::BindUtil::bind(
        &printCategoryWithDelimiter,
        bsl::shared_ptr<bsl::ostream>(&os, bslstl::SharedPtrNilDeleter()),
        bdlf::PlaceHolders::_1,  // category
        ':'));

    return 0;
}

int LogController::processVerbCommand(bsl::istream& cmd, bsl::ostream& os)
{
    bsl::string severity;
    cmd >> severity;

    if (cmd.fail()) {
        os << "Missing <severity> parameter";
        return -1;  // RETURN
    }

    ball::Severity::Level level;
    if (ball::SeverityUtil::fromAsciiCaseless(&level, severity.c_str()) != 0) {
        os << "Invalid severity '" << severity << "'";
        return -2;  // RETURN
    }

    setVerbosityLevel(level);
    os << "Log verbosity set to '" << level << "'";

    return 0;
}

int LogController::processConsoleCommand(bsl::istream& cmd, bsl::ostream& os)
{
    bsl::string severity;
    cmd >> severity;

    if (cmd.fail()) {
        os << "Missing <severity> parameter";
        return -1;  // RETURN
    }

    ball::Severity::Level level;
    if (ball::SeverityUtil::fromAsciiCaseless(&level, severity.c_str()) != 0) {
        os << "Invalid severity '" << severity << "'";
        return -2;  // RETURN
    }

    d_consoleObserver.setSeverityThreshold(level);
    os << "Console severity threshold set to '" << level << "'";

    // In order for a trace to be printed by the console observer, two
    // conditions must be met:
    // - the record's severity must be greater than or equal to the log
    //   verbosity,
    // - and the console observer's threshold severity must also be greater
    //   than or equal to the log record severity.
    // If the log verbosity is info, setting console log threshold to 'debug'
    // will not have the expected desired effect..., emit a note in that case.
    ball::Severity::Level logVerbosity = static_cast<ball::Severity::Level>(
        ball::Administration::defaultPassThresholdLevel());
    if (logVerbosity < level) {
        os << ".  Note that the current verbosity (" << logVerbosity << ") is "
           << "higher than the new console severity threshold, use the "
           << "'LOG VERB <severity>' command to increase the log verbosity if "
           << "needed.";
    }

    return 0;
}

int LogController::processCategoryCommand(bsl::istream& cmd, bsl::ostream& os)
{
    bsl::string category;
    bsl::string severity;
    bsl::string color;

    cmd >> category;
    if (cmd.fail()) {
        os << "Missing <category> parameter";
        return -1;  // RETURN
    }
    cmd >> severity;
    if (cmd.fail()) {
        os << "Missing <severity> parameter";
        return -2;  // RETURN
    }
    cmd >> color;  // Color is an optional parameter ..

    ball::Severity::Level level;
    if (ball::SeverityUtil::fromAsciiCaseless(&level, severity.c_str()) != 0) {
        os << "Invalid severity '" << severity << "'";
        return -3;  // RETURN
    }

    setCategoryVerbosity(category, level);
    d_consoleObserver.setCategoryColor(category, color);
    // Don't check for color.empty(), an empty string, per
    // 'ConsoleObserver::setCategoryColor' contract will delete color
    // information associated to this category.

    os << "Verbosity of category '" << category << "' set to " << level;

    return 0;
}

int LogController::tryRegisterObserver(ball::Observer* observer)
{
    int rc = d_multiplexObserver.registerObserver(observer);

    if (rc != 0) {
        for (size_t i = 0; i < d_registeredObservers.size(); i++) {
            d_multiplexObserver.deregisterObserver(d_registeredObservers[i]);
        }
        d_registeredObservers.clear();
    }
    else {
        d_registeredObservers.push_back(observer);
    }

    return rc;
}

LogController::LogController(bdlmt::EventScheduler* scheduler,
                             bslma::Allocator*      allocator)
: d_allocator_p(allocator)
, d_isInitialized(false)
, d_config(allocator)
, d_multiplexObserver(allocator)
, d_fileObserver(ball::Severity::OFF,  // disable stdout (we use Console)
                 false,                // use UTC time
                 8192,                 // maxQueueLogEntries
                 allocator)
, d_alarmLog(allocator)
, d_consoleObserver(allocator)
, d_syslogObserver(allocator)
, d_logCleaner(scheduler, allocator)
, d_lastLogLinkPath(allocator)
, d_registeredObservers(allocator)
{
    // NOTHING
}

LogController::~LogController()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isInitialized &&
                    "shutdown() must be called before destroying this object");
}

int LogController::initialize(bsl::ostream&              errorDescription,
                              const LogControllerConfig& config)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isInitialized &&
                    "initialize() can only be called once on this object");

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                             = 0,
        rc_LOGGERMANAGER_ALREADY_INITIALIZED   = -1,
        rc_LOGGERMANAGER_CONFIGURE_FAILED      = -2,
        rc_FILEOBSERVER_ENABLE_FAILED          = -3,
        rc_FILEOBSERVER_STARTTHREAD_FAILED     = -4,
        rc_FILEOBSERVER_REGISTRATION_FAILED    = -5,
        rc_ALARMLOG_REGISTRATION_FAILED        = -6,
        rc_CONSOLEOBSERVER_REGISTRATION_FAILED = -7,
        rc_SYSLOGOBSERVER_REGISTRATION_FAILED  = -8
    };

    if (ball::LoggerManager::isInitialized()) {
        errorDescription << "ball::LoggerManager is already initialized, "
                         << "LogController can't be used";
        return rc_LOGGERMANAGER_ALREADY_INITIALIZED;  // RETURN
    }

    int rc = 0;

    // -------------
    // LoggerManager
    ball::LoggerManagerConfiguration lmc;
    lmc.setLogOrder(ball::LoggerManagerConfiguration::LIFO);
    lmc.setDefaultThresholdLevelsCallback(bdlf::BindUtil::bind(
        &ball::LoggerFunctorPayloads::loadParentCategoryThresholdValues,
        bdlf::PlaceHolders::_1,
        bdlf::PlaceHolders::_2,
        bdlf::PlaceHolders::_3,
        bdlf::PlaceHolders::_4,
        bdlf::PlaceHolders::_5,
        '.'));
    rc = lmc.setDefaultRecordBufferSizeIfValid(32768);
    if (rc != 0) {
        errorDescription << "Unable to set default record buffer size on lmc "
                         << "[rc: " << rc << "]";
        return rc_LOGGERMANAGER_CONFIGURE_FAILED;  // RETURN
    }

    ball::LoggerManager::initSingleton(&d_multiplexObserver,
                                       lmc,
                                       d_allocator_p);

    // ------------
    // FileObserver
    rc = d_fileObserver.enableFileLogging(config.fileName().c_str());
    if (rc != 0) {
        errorDescription << "Failed enabling file logging "
                         << "[rc: " << rc << ", file: '" << config.fileName()
                         << "']";
        ball::LoggerManager::shutDownSingleton();
        return rc_FILEOBSERVER_ENABLE_FAILED;  // RETURN
    }

    d_fileObserver.setLogFormat(config.logfileFormat().c_str(),
                                config.consoleFormat().c_str());
    d_fileObserver.rotateOnSize(config.rotationBytes() / 1024);
    d_fileObserver.rotateOnTimeInterval(
        bdlt::DatetimeInterval(0, 0, 0, config.rotationSeconds()));

    rc = d_fileObserver.startPublicationThread();
    if (rc != 0) {
        errorDescription << "Failed to start FileObserver publication thread "
                         << "[rc: " << rc << "]";
        ball::LoggerManager::shutDownSingleton();
        return rc_FILEOBSERVER_STARTTHREAD_FAILED;  // RETURN
    }

    rc = tryRegisterObserver(&d_fileObserver);
    if (rc != 0) {
        errorDescription << "Failed registering FileObserver "
                         << "[rc: " << rc << "]";
        ball::LoggerManager::shutDownSingleton();
        return rc_FILEOBSERVER_REGISTRATION_FAILED;  // RETURN
    }

    // --------
    // AlarmLog
    rc = tryRegisterObserver(&d_alarmLog);
    if (rc != 0) {
        errorDescription << "Failed registering AlarmLog "
                         << "[rc: " << rc << "]";
        ball::LoggerManager::shutDownSingleton();
        return rc_ALARMLOG_REGISTRATION_FAILED;  // RETURN
    }

    // ---------
    // bsls::Log
    bsls::Log::setSeverityThreshold(config.bslsLogSeverityThreshold());
    bsls::Log::setLogMessageHandler(bslsLogHandler);

    // ---------------
    // ConsoleObserver
    d_consoleObserver.setSeverityThreshold(config.consoleSeverityThreshold())
        .setLogFormat(config.consoleFormat());

    rc = tryRegisterObserver(&d_consoleObserver);
    if (rc != 0) {
        errorDescription << "Failed registering ConsoleObserver "
                         << "[rc: " << rc << "]";
        ball::LoggerManager::shutDownSingleton();
        return rc_CONSOLEOBSERVER_REGISTRATION_FAILED;  // RETURN
    }

    // ---------------
    // SyslogObserver
    if (config.syslogEnabled()) {
        d_syslogObserver.setSeverityThreshold(config.syslogVerbosity())
            .setLogFormat(config.syslogFormat());
        d_syslogObserver.enableLogging(config.syslogAppName());

        rc = tryRegisterObserver(&d_syslogObserver);
        if (rc != 0) {
            errorDescription << "Failed registering SyslogObserver "
                             << "[rc: " << rc << "]";
            ball::LoggerManager::shutDownSingleton();
            return rc_SYSLOGOBSERVER_REGISTRATION_FAILED;  // RETURN
        }
    }

    // -------------
    // Configuration
    setVerbosityLevel(config.loggingVerbosity());

    const LogControllerConfig::CategoryPropertiesMap& categories =
        config.categoriesProperties();
    for (LogControllerConfig::CategoryPropertiesMap::const_iterator it =
             categories.begin();
         it != categories.end();
         ++it) {
        setCategoryVerbosity(it->first, it->second.d_verbosity);
        d_consoleObserver.setCategoryColor(it->first, it->second.d_color);
    }

    // ----------
    // LogCleanup
    if (config.fileMaxAgeDays() != 0) {
        bsl::string filePattern;
        ball::LogFileCleanerUtil::logPatternToFilePattern(&filePattern,
                                                          config.fileName());

        bsls::TimeInterval maxAge(0, 0);
        maxAge.addDays(config.fileMaxAgeDays());

        rc = d_logCleaner.start(filePattern, maxAge);
        if (rc != 0) {
            BALL_LOG_ERROR << "Failed to start log cleaning of '"
                           << filePattern << "' [rc: " << rc << "]";
        }
    }
    else {
        BALL_LOG_INFO << "LogCleaning is *disabled* "
                      << "[reason: 'fileMaxAgeDays' is set to 0 in config]";
    }

    // --------------------
    // LastLog symlink path
    ball::LogFileCleanerUtil::logPatternToFilePattern(&d_lastLogLinkPath,
                                                      config.fileName());
    // The above replaced all %x by *, so 'logs.%T.%p' now looks like
    // "logs.*.*".  Remove all ".*" from the input, as we need to 'guess' the
    // base filename of the logs (here we are assuming that the substitution
    // pattern are commonly prefixed by a dot).
    for (size_t idx = d_lastLogLinkPath.find(".*"); idx != bsl::string::npos;
         idx        = d_lastLogLinkPath.find(".*")) {
        d_lastLogLinkPath.erase(idx, 2);
    }
    // 'logPatternToFilePattern', might potentially append a single '*' at the
    // end of the file, so remove it.
    if (d_lastLogLinkPath[d_lastLogLinkPath.length() - 1] == '*') {
        d_lastLogLinkPath.erase(d_lastLogLinkPath.length() - 1, 1);
    }

    // Append .last
    d_lastLogLinkPath.append(".last");

    // Register log rotation callback
    d_fileObserver.setOnFileRotationCallback(
        bdlf::BindUtil::bind(&LogController::afterFileRotation,
                             this,
                             bdlf::PlaceHolders::_1,    // status
                             bdlf::PlaceHolders::_2));  // rotatedFileName

    updateLastLogSymlink();  // Force creation of the .last now

    d_isInitialized = true;
    d_config        = config;
    return rc_SUCCESS;
}

void LogController::shutdown()
{
    if (!d_isInitialized) {
        return;  // RETURN
    }

    d_logCleaner.stop();

    // Unregister all observers
    d_multiplexObserver.deregisterObserver(&d_consoleObserver);
    d_multiplexObserver.deregisterObserver(&d_alarmLog);
    d_multiplexObserver.deregisterObserver(&d_fileObserver);
    d_fileObserver.stopPublicationThread();

    // Shutdown logger manager
    ball::LoggerManager::shutDownSingleton();

    d_isInitialized = false;
}

void LogController::setVerbosityLevel(ball::Severity::Level verbosity)
{
    ball::Administration::setDefaultThresholdLevels(
        ball::Severity::OFF,   // recording level
        verbosity,             // passthrough level
        ball::Severity::OFF,   // trigger level
        ball::Severity::OFF);  // triggerAll level
    ball::Administration::setThresholdLevels(
        "*",
        ball::Severity::OFF,   // recording level
        verbosity,             // passthrough level
        ball::Severity::OFF,   // trigger level
        ball::Severity::OFF);  // triggerAll level
}

void LogController::setCategoryVerbosity(const bsl::string&    category,
                                         ball::Severity::Level verbosity)
{
    bsl::string categoryNoStar(category);
    if (category.length() > 1 && category[category.length() - 1] == '*') {
        categoryNoStar.erase(category.length() - 1);
    }

    ball::Administration::addCategory(
        categoryNoStar.c_str(),
        ball::Severity::OFF,   // recording level
        verbosity,             // passthrough level
        ball::Severity::OFF,   // trigger level
        ball::Severity::OFF);  // triggerAll level

    ball::Administration::setThresholdLevels(
        category.c_str(),
        ball::Severity::OFF,   // recording level
        verbosity,             // passthrough level
        ball::Severity::OFF,   // trigger level
        ball::Severity::OFF);  // triggerAll level
}

int LogController::processCommand(const bsl::string& cmd, bsl::ostream& os)
{
    // Extract the first word of the command, i.e., the action
    bsl::istringstream is(cmd);
    bsl::string        action;
    is >> action;

    int rc = 0;
    if (bdlb::String::areEqualCaseless(action, "HELP")) {
        os << "This process responds to the following LOG subcommands:\n"
           << "    HELP\n"
           << "        Display this message\n"
           << "    INFO\n"
           << "        Print information about current logging status\n"
           << "    VERB <severity>\n"
           << "        Set the verbosity to <severity>\n"
           << "    CONSOLE <severity>\n"
           << "        Set the console output verbosity to <severity>\n"
           << "    CATEGORY <category> <severity> [<color>]\n"
           << "        Set the verbosity of the <category> category expression"
              " to <severity> with the optional <color>\n"
           << "\n"
           << "    Where severity is one of: "
              "'OFF|TRACE|DEBUG|INFO|WARN|ERROR|FATAL'";
    }
    else if (bdlb::String::areEqualCaseless(action, "INFO")) {
        rc = processInfoCommand(is, os);
    }
    else if (bdlb::String::areEqualCaseless(action, "VERB")) {
        rc = processVerbCommand(is, os);
    }
    else if (bdlb::String::areEqualCaseless(action, "CONSOLE")) {
        rc = processConsoleCommand(is, os);
    }
    else if (bdlb::String::areEqualCaseless(action, "CATEGORY")) {
        rc = processCategoryCommand(is, os);
    }
    else {
        os << "Unknown 'LOG' command '" << cmd << "'; "
           << "see 'LOG HELP' for the list of valid commands.";
        return -1;  // RETURN
    }

    return rc;
}

}  // close package namespace
}  // close enterprise namespace
