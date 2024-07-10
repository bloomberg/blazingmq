// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mqbcmd_parseutil.cpp                                               -*-C++-*-
#include <mqbcmd_parseutil.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcmd_messages.h>

// MWC
#include <mwcu_memoutstream.h>
#include <mwcu_stringutil.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <bdlb_numericparseutil.h>
#include <bdlb_stringrefutil.h>
#include <bdlsb_fixedmeminstreambuf.h>
#include <bdlt_iso8601util.h>
#include <bsl_istream.h>
#include <bsl_ostream.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbcmd {

namespace {

/// This function is provided as a shorthand.
bool equalCaseless(const bslstl::StringRef& lhs, const bslstl::StringRef& rhs)
{
    return bdlb::StringRefUtil::areEqualCaseless(lhs, rhs);
}

/// This `class` provides a function-like cursor over a contiguous sequence
/// of non-empty `const bslstl::StringRef` objects.
class WordGenerator {
  private:
    const bslstl::StringRef* d_current_p;
    const bslstl::StringRef* d_end_p;

  public:
    WordGenerator(const bslstl::StringRef* current,
                  const bslstl::StringRef* end)
    : d_current_p(current)
    , d_end_p(end)
    {
        // NOTHING
    }

    /// Return the current word referred to by this generator.  If there are
    /// no more words, return the empty string.
    bslstl::StringRef operator()()
    {
        if (d_current_p == d_end_p) {
            return bslstl::StringRef();  // RETURN
        }
        else {
            return *d_current_p++;  // RETURN
        }
    }
};

// 'parseInt', 'parseInt64', and 'parseDouble', below, are wrappers around
// their 'bdlb::NumericParseUtil' counterparts, except that the versions
// defined here require that the entire input string is consumed by the parse.

#define DEFINE_STRICT_NUMERIC_PARSER(NAME, TYPE)                              \
    int NAME(TYPE* result, const bslstl::StringRef& inputString)              \
    {                                                                         \
        bslstl::StringRef remainder;                                          \
        const int         rc = bdlb::NumericParseUtil::NAME(result,           \
                                                    &remainder,       \
                                                    inputString);     \
                                                                              \
        if (rc) {                                                             \
            return rc;                                                        \
        }                                                                     \
                                                                              \
        /* empty() == true -> false -> 0 -> success */                        \
        return !remainder.empty();                                            \
    }

DEFINE_STRICT_NUMERIC_PARSER(parseInt, int)
DEFINE_STRICT_NUMERIC_PARSER(parseInt64, bsls::Types::Int64)
DEFINE_STRICT_NUMERIC_PARSER(parseDouble, double)

#undef DEFINE_STRICT_NUMERIC_PARSER

/// Parse a `Value` from the specified `text`. The following conversions are
/// attempted -- the first to succeed determines the value returned:
/// * "null" results in a value with the selection `theNull`.
/// * "true" or "false" results in a value with the selection `theBool`.
/// * An integer results in a value with the selection `theInteger`.
/// * A floating point number results in a value with the selection
///   `theDouble`. This includes infinite and not-a-number representations.
/// * An ISO 8601 date, time, or datetime results in a value with the
///   selection of `theDate`, `theTime`, or `theDatetime`, respectively.
/// * Any other input results in a value with the selection `theString`,
///   containing the value of `text`.
///
/// Note that integers and floating point numbers are parsed according to
/// the grammar described in the documentation of the component
/// `bdlb_numericparseutil`. Also note that the returned `Value` always has
/// a valid selection (i.e. the default constructed `Value` is never
/// returned).
Value parseValue(const bslstl::StringRef& text)
{
    Value result;

    if (text == "null") {
        result.makeTheNull();
        return result;  // RETURN
    }
    else if (text == "true") {
        result.makeTheBool(true);
        return result;  // RETURN
    }
    else if (text == "false") {
        result.makeTheBool(false);
        return result;  // RETURN
    }
    else if (parseInt64(&result.makeTheInteger(), text) == 0) {
        return result;  // RETURN
    }
    else if (parseDouble(&result.makeTheDouble(), text) == 0) {
        return result;  // RETURN
    }
    else if (bdlt::Iso8601Util::parse(&result.makeTheDate(), text) == 0) {
        return result;  // RETURN
    }
    else if (bdlt::Iso8601Util::parse(&result.makeTheTime(), text) == 0) {
        return result;  // RETURN
    }
    else if (bdlt::Iso8601Util::parse(&result.makeTheDatetime(), text) == 0) {
        return result;  // RETURN
    }

    result.makeTheString() = text;

    return result;
}

/// Verify that the specified `next` will yield no more nonempty strings.
/// Return zero if `next` is finished, or return a nonzero value if `next`
/// can yield more nonempty strings.  If `next` is not finished, load a
/// diagnostic message into the specified `error`.
int expectEnd(bsl::string* error, WordGenerator next)
{
    bslstl::StringRef token = next();
    if (token.empty()) {
        return 0;  // RETURN
    }

    *error = "Unexpected trailing words in an otherwise well-formed command: "
             "The extra words are:";
    do {
        *error += " ";
        *error += token;
        token = next();
    } while (!token.empty());

    return -1;
}

// What follows are declarations of all of the parsing implementation
// functions, followed by their definitions.  Since all methods have the same
// signature, except for the type of the first parameter, we use a convenient
// shortcut macro.
#define DEF_FUNC(NAME, TYPE)                                                  \
    int parse##NAME(TYPE* command, bsl::string* error, WordGenerator next)
// Load into the specified 'command' an object parsed from words supplied
// by the specified 'next' function.  Return zero on success or a nonzero
// value otherwise.  If an error occurs, load a diagnostic into the
// specified 'error'.

DEF_FUNC(Command, Command);
DEF_FUNC(Help, HelpCommand);
DEF_FUNC(DomainsCommand, DomainsCommand);
DEF_FUNC(DomainCommand, Domain);
DEF_FUNC(DomainQueue, DomainQueue);
DEF_FUNC(DomainQueuePurge, QueueCommand);
DEF_FUNC(DomainQueueList, ListMessages);
DEF_FUNC(DomainReconfigure, DomainReconfigure);
DEF_FUNC(DomainResolver, DomainResolverCommand);
DEF_FUNC(ConfigProvider, ConfigProviderCommand);
DEF_FUNC(Stat, StatCommand);
DEF_FUNC(BrokerConfig, BrokerConfigCommand);
DEF_FUNC(ClustersCommand, ClustersCommand);
DEF_FUNC(AddReverseProxy, AddReverseProxy);
DEF_FUNC(Cluster, Cluster);
DEF_FUNC(Storage, ClusterCommand);
DEF_FUNC(StoragePartition, StoragePartition);
DEF_FUNC(StorageDomain, StorageDomain);
DEF_FUNC(ClusterState, ClusterCommand);
DEF_FUNC(Elector, ElectorCommand);
DEF_FUNC(Danger, DangerCommand);
DEF_FUNC(Replication, ReplicationCommand);
#undef DEF_FUNC

/// Load into the specified `command` an object parsed from words supplied
/// by the specified `next` function.  Return zero on success or a nonzero
/// value otherwise.  If an error occurs, load a diagnostic into the
/// specified `error`.  Use the optionally specified `commandPrefix` in the
/// error diagnostic.
int parseClearCache(ClearCache*              command,
                    bsl::string*             error,
                    WordGenerator            next,
                    const bslstl::StringRef& commandPrefix);

int parseEncodingFormat(EncodingFormat::Value* format,
                        bsl::string*           error,
                        WordGenerator&         next);

/// entry point (i.e. `root`) of the command parsing
int parseCommand(Command* command, bsl::string* error, WordGenerator next)
{
    bslstl::StringRef word = next();
    if (word.empty()) {
        *error = "Command must contain at least one word.";
        return -1;  // RETURN
    }

    if (equalCaseless(word, "ENCODING")) {
        if (0 != parseEncodingFormat(&command->encoding(), error, next)) {
            return -1;  // RETURN
        }

        word = next();
        if (word.empty()) {
            *error = "Command must contain at least one word.";
            return -1;  // RETURN
        }
    }

    if (equalCaseless(word, "HELP")) {
        return parseHelp(&command->choice().makeHelp(),
                         error,
                         next);  // RETURN
    }
    else if (equalCaseless(word, "DOMAINS")) {
        return parseDomainsCommand(&command->choice().makeDomains(),
                                   error,
                                   next);  // RETURN
    }
    else if (equalCaseless(word, "CONFIGPROVIDER")) {
        return parseConfigProvider(&command->choice().makeConfigProvider(),
                                   error,
                                   next);  // RETURN
    }
    else if (equalCaseless(word, "STAT")) {
        return parseStat(&command->choice().makeStat(),
                         error,
                         next);  // RETURN
    }
    else if (equalCaseless(word, "CLUSTERS")) {
        return parseClustersCommand(&command->choice().makeClusters(),
                                    error,
                                    next);
        // RETURN
    }
    else if (equalCaseless(word, "DANGER")) {
        return parseDanger(&command->choice().makeDanger(),
                           error,
                           next);  // RETURN
    }
    else if (equalCaseless(word, "BROKERCONFIG")) {
        return parseBrokerConfig(&command->choice().makeBrokerConfig(),
                                 error,
                                 next);  // RETURN
    }

    *error = "Invalid command. Send \"HELP\" for list of commands. Invalid "
             "command word: " +
             word;
    return -1;
}

// ENCODING ...
int parseEncodingFormat(EncodingFormat::Value* format,
                        bsl::string*           error,
                        WordGenerator&         next)
{
    const bslstl::StringRef word = next();

    const char errorCommon[] = "The ENCODING option must be followed by one "
                               "of the [TEXT, JSON_COMPACT, JSON_PRETTY] "
                               "settings (default: TEXT)";

    if (word.empty()) {
        *error += errorCommon;
        *error += ", but no setting was specified.";
        return -1;  // RETURN
    }

    if (equalCaseless(word, "TEXT")) {
        *format = EncodingFormat::TEXT;
        return 0;  // RETURN
    }
    else if (equalCaseless(word, "JSON_COMPACT")) {
        *format = EncodingFormat::JSON_COMPACT;
        return 0;  // RETURN
    }
    else if (equalCaseless(word, "JSON_PRETTY")) {
        *format = EncodingFormat::JSON_PRETTY;
        return 0;  // RETURN
    }

    *error = errorCommon;
    *error += ", but the following was given: " + word;
    return -1;
}

/// HELP ...
int parseHelp(HelpCommand* command, bsl::string* error, WordGenerator next)
{
    const bslstl::StringRef word = next();

    if (word.empty()) {
        return 0;  // RETURN
    }

    if (equalCaseless(word, "PLUMBING")) {
        command->plumbing() = true;
        return expectEnd(error, next);  // RETURN
    }

    *error = "Invalid HELP subcommand: " + word;
    return -1;
}

/// DOMAINS ...
int parseDomainsCommand(DomainsCommand* domains,
                        bsl::string*    error,
                        WordGenerator   next)
{
    const bslstl::StringRef word = next();

    const char errorCommon[] = "The DOMAINS command must be followed by one "
                               "of the [DOMAIN, RESOLVER, RECONFIGURE] "
                               "subcommands";

    if (word.empty()) {
        *error += errorCommon;
        *error += ", but no subcommand was specified.";
        return -1;  // RETURN
    }

    if (equalCaseless(word, "DOMAIN")) {
        return parseDomainCommand(&domains->makeDomain(),
                                  error,
                                  next);  // RETURN
    }
    else if (equalCaseless(word, "RESOLVER")) {
        return parseDomainResolver(&domains->makeResolver(),
                                   error,
                                   next);  // RETURN
    }
    else if (equalCaseless(word, "RECONFIGURE")) {
        return parseDomainReconfigure(&domains->makeReconfigure(),
                                      error,
                                      next);  // RETURN
    }

    *error = errorCommon;
    *error += ", but the following was given: " + word;
    return -1;
}

/// DOMAINS DOMAIN ...
int parseDomainCommand(Domain* domain, bsl::string* error, WordGenerator next)
{
    const bslstl::StringRef name = next();

    if (name.empty()) {
        *error = "The DOMAINS DOMAIN command must be followed by a domain "
                 "name.";
        return -1;  // RETURN
    }

    domain->name() = name;

    const bslstl::StringRef subCommand = next();

    if (subCommand.empty()) {
        *error = "The DOMAINS DOMAIN <name> command must be followed by a "
                 "subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subCommand, "PURGE")) {
        domain->command().makePurge();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subCommand, "INFOS")) {
        domain->command().makeInfo();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subCommand, "QUEUE")) {
        return parseDomainQueue(&domain->command().makeQueue(), error, next);
        // RETURN
    }

    *error = "Invalid DOMAIN subcommand: " + subCommand;
    return -1;
}

/// DOMAINS DOMAIN <name> QUEUE ...
int parseDomainQueue(DomainQueue*  queue,
                     bsl::string*  error,
                     WordGenerator next)
{
    const bslstl::StringRef name = next();

    if (name.empty()) {
        *error = "DOMAINS DOMAIN <name> QUEUE command must specify a queue "
                 "name.";
        return -1;  // RETURN
    }

    queue->name() = name;

    const bslstl::StringRef subcommand = next();

    if (name.empty()) {
        *error = "DOMAINS DOMAIN <name> QUEUE <queue_name> command must have "
                 "a subcommand, such as PURGE, INTERNALS, or LIST.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "PURGE")) {
        return parseDomainQueuePurge(&queue->command(),
                                     error,
                                     next);  // RETURN
    }
    else if (equalCaseless(subcommand, "INTERNALS")) {
        queue->command().makeInternals();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "LIST")) {
        return parseDomainQueueList(&queue->command().makeMessages(),
                                    error,
                                    next);  // RETURN
    }

    *error = "Invalid DOMAINS DOMAIN <name> QUEUE <queue_name> subcommand: " +
             subcommand;
    return -1;
}

/// DOMAINS DOMAIN <name> QUEUE <queue_name> PURGE
int parseDomainQueuePurge(QueueCommand* command,
                          bsl::string*  error,
                          WordGenerator next)
{
    const bslstl::StringRef appId = next();

    if (appId.empty()) {
        *error = "DOMAINS DOMAIN <name> QUEUE <queue_name> PURGE command must "
                 "be followed by an appId or \"*\".";
        return -1;  // RETURN
    }

    command->makePurgeAppId(appId);
    return expectEnd(error, next);
}

/// DOMAINS DOMAIN <name> QUEUE <queue_name> LIST ...
int parseDomainQueueList(ListMessages* command,
                         bsl::string*  error,
                         WordGenerator next)
{
    // [<appId>] <offset> <count>
    // Since the appId is optional, we don't know how to interpret the
    // arguments until we know whether there are two or three.
    const bslstl::StringRef word1 = next();
    const bslstl::StringRef word2 = next();
    const bslstl::StringRef word3 = next();

    bslstl::StringRef offsetString, countString;

    if (word3.empty()) {
        // appId is not specified
        offsetString = word1;
        countString  = word2;
    }
    else {
        command->appId() = word1;
        offsetString     = word2;
        countString      = word3;
    }

    if (parseInt(&command->offset(), offsetString)) {
        *error = "Invalid <offset> for DOMAINS DOMAIN <name> QUEUE "
                 "<queue_name> LIST [<appId>] <offset> <count>: " +
                 offsetString;
        return -1;  // RETURN
    }

    if (equalCaseless(countString, "unlimited")) {
        command->count() = 0;
    }
    else if (parseInt(&command->count(), countString)) {
        *error = "Invalid <count> for DOMAINS DOMAIN <name> QUEUE "
                 "<queue_name> LIST [<appId>] <offset> <count>: " +
                 countString;
        return -1;  // RETURN
    }

    return expectEnd(error, next);
}

/// DOMAINS RESOLVER ...
int parseDomainResolver(DomainResolverCommand* command,
                        bsl::string*           error,
                        WordGenerator          next)
{
    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "DOMAINS RESOLVER command must be followed by a subcommand.";
        return -1;  // RETURN
    }

    if (!equalCaseless(subcommand, "CACHE_CLEAR")) {
        *error = "Invalid DOMAINS RESOLVER subcommand: " + subcommand;
        return -1;  // RETURN
    }

    return parseClearCache(&command->makeClearCache(),
                           error,
                           next,
                           "DOMAINS RESOLVER CACHE_CLEAR");
}

/// ... RECONFIGURE ...
int parseDomainReconfigure(DomainReconfigure* reconfigure,
                           bsl::string*       error,
                           WordGenerator      next)
{
    const bslstl::StringRef domain = next();

    if (domain.empty()) {
        *error = "DOMAINS RECONFIGURE command must be followed by a "
                 "domain name.";
        return -1;  // RETURN
    }
    reconfigure->makeDomain(domain);

    return expectEnd(error, next);
}

/// ... CACHE_CLEAR ...
int parseClearCache(ClearCache*              clear,
                    bsl::string*             error,
                    WordGenerator            next,
                    const bslstl::StringRef& command)
{
    // 'command' is just for the error message

    const bslstl::StringRef domain = next();

    if (domain.empty()) {
        *error = command +
                 " command must be followed by a domain name or \"ALL\".";
        return -1;  // RETURN
    }

    if (equalCaseless(domain, "ALL")) {
        clear->makeAll();
    }
    else {
        clear->makeDomain(domain);
    }

    return expectEnd(error, next);
}

/// CONFIGPROVIDER ...
int parseConfigProvider(ConfigProviderCommand* command,
                        bsl::string*           error,
                        WordGenerator          next)
{
    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "CONFIGPROVIDER command must be followed by a subcommand.";
        return -1;  // RETURN
    }

    if (!equalCaseless(subcommand, "CACHE_CLEAR")) {
        *error = "Invalid CONFIGPROVIDER subcommand: " + subcommand;
    }

    return parseClearCache(&command->makeClearCache(),
                           error,
                           next,
                           "CONFIGPROVIDER CACHE_CLEAR");
}

/// STAT ...
int parseStat(StatCommand* stats, bsl::string* error, WordGenerator next)
{
    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "STAT command must be followed by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "SHOW")) {
        stats->makeShow();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "SET")) {
        const bslstl::StringRef parameter = next();

        if (parameter.empty()) {
            *error = "The command STAT SET "
                     "must be followed by a parameter name.";
            return -1;  // RETURN
        }

        const bslstl::StringRef valueString = next();

        if (valueString.empty()) {
            *error = "The command STAT SET <parameter> "
                     "must be followed by a new value for the parameter.";
            return -1;  // RETURN
        }

        SetTunable& tunable = stats->makeSetTunable();
        tunable.name()      = parameter;
        tunable.value()     = parseValue(valueString);

        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "GET")) {
        const bslstl::StringRef parameter = next();

        if (parameter.empty()) {
            *error = "The command STAT GET "
                     "must be followed by a parameter name.";
            return -1;  // RETURN
        }

        stats->makeGetTunable(parameter);
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "LIST_TUNABLES")) {
        stats->makeListTunables();
        return expectEnd(error, next);  // RETURN
    }

    *error = "Unexpected STAT subcommand: " + subcommand;
    return -1;
}

/// BROKERCONFIG ...
int parseBrokerConfig(BrokerConfigCommand* brokerConfig,
                      bsl::string*         error,
                      WordGenerator        next)
{
    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "BROKERCONFIG command must be followed by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "DUMP")) {
        brokerConfig->makeDump();
        return expectEnd(error, next);  // RETURN
    }

    *error = "Unexpected BROKERCONFIG subcommand: " + subcommand;
    return -1;
}

/// CLUSTERS ...
int parseClustersCommand(ClustersCommand* clusters,
                         bsl::string*     error,
                         WordGenerator    next)
{
    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "CLUSTERS command must be followed by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "LIST")) {
        clusters->makeList();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "ADDREVERSE")) {
        return parseAddReverseProxy(&clusters->makeAddReverseProxy(),
                                    error,
                                    next);  // RETURN
    }
    else if (equalCaseless(subcommand, "CLUSTER")) {
        return parseCluster(&clusters->makeCluster(), error, next);  // RETURN
    }

    *error = "Invalid CLUSTERS subcommand: " + subcommand;
    return -1;
}

/// CLUSTERS ADDREVERSE ...
int parseAddReverseProxy(AddReverseProxy* command,
                         bsl::string*     error,
                         WordGenerator    next)
{
    command->clusterName() = next();
    command->remotePeer()  = next();

    if (command->clusterName().empty() || command->remotePeer().empty()) {
        *error = "CLUSTERS ADDREVERSE <clusterName> <remotePeer> requires two "
                 "arguments.";
        return -1;  // RETURN
    }

    return expectEnd(error, next);
}

/// CLUSTERS CLUSTER ...
int parseCluster(Cluster* cluster, bsl::string* error, WordGenerator next)
{
    cluster->name() = next();
    if (cluster->name().empty()) {
        *error = "CLUSTERS CLUSTER command must be followed by a cluster "
                 "name.";
        return -1;  // RETURN
    }

    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "CLUSTERS CLUSTER <name> must be followed by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "STATUS")) {
        cluster->command().makeStatus();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "QUEUEHELPER")) {
        cluster->command().makeQueueHelper();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "FORCE_GC_QUEUES")) {
        cluster->command().makeForceGcQueues();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "STORAGE")) {
        return parseStorage(&cluster->command(), error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "STATE")) {
        return parseClusterState(&cluster->command(), error, next);  // RETURN
    }

    *error = "Invalid subcommand after CLUSTERS CLUSTER <name>: " + subcommand;
    return -1;
}

/// CLUSTERS CLUSTER <name> STORAGE ...
int parseStorage(ClusterCommand* command,
                 bsl::string*    error,
                 WordGenerator   next)
{
    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "The command CLUSTERS CLUSTER <name> STORAGE must be "
                 "followed by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "SUMMARY")) {
        command->makeStorage().makeSummary();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "PARTITION")) {
        return parseStoragePartition(&command->makeStorage().makePartition(),
                                     error,
                                     next);  // RETURN
    }
    else if (equalCaseless(subcommand, "DOMAIN")) {
        return parseStorageDomain(&command->makeStorage().makeDomain(),
                                  error,
                                  next);  // RETURN
    }
    else if (equalCaseless(subcommand, "REPLICATION")) {
        return parseReplication(&command->makeStorage().makeReplication(),
                                error,
                                next);  // RETURN
    }

    *error = "Invalid subcommand after CLUSTERS CLUSTER <name> STORAGE: " +
             subcommand;
    return -1;  // RETURN
}

/// CLUSTERS CLUSTER <name> STORAGE PARTITION ...
int parseStoragePartition(StoragePartition* partition,
                          bsl::string*      error,
                          WordGenerator     next)
{
    const bslstl::StringRef partitionIdString = next();

    if (partitionIdString.empty()) {
        *error = "The command CLUSTERS CLUSTER <name> STORAGE PARTITION must "
                 "be followed by a <partitionId>.";
        return -1;  // RETURN
    }

    if (parseInt(&partition->partitionId(), partitionIdString)) {
        *error = "Invalid <partitionId> in command CLUSTERS CLUSTER <name> "
                 "STORAGE PARTITION <partitionId>: " +
                 partitionIdString;
        return -1;  // RETURN
    }

    StoragePartitionCommand& command    = partition->command();
    const bslstl::StringRef  subcommand = next();

    if (subcommand.empty()) {
        *error = "The command CLUSTERS CLUSTER <name> STORAGE PARTITION "
                 "<partitionId> must be followed by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "ENABLE")) {
        command.makeEnable();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "DISABLE")) {
        command.makeDisable();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "SUMMARY")) {
        command.makeSummary();
        return expectEnd(error, next);  // RETURN
    }

    *error = "Invalid subcommand after CLUSTERS CLUSTER <name> STORAGE "
             "PARTITION <partitionId>: " +
             subcommand;
    return -1;
}

/// CLUSTERS CLUSTER <name> STORAGE DOMAIN ...
int parseStorageDomain(StorageDomain* domain,
                       bsl::string*   error,
                       WordGenerator  next)
{
    domain->name() = next();
    if (domain->name().empty()) {
        *error = "The command CLUSTERS CLUSTER <name> STORAGE DOMAIN must be "
                 "followed by a domain name.";
        return -1;  // RETURN
    }

    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "The command CLUSTERS CLUSTER <name> STORAGE DOMAIN "
                 "<domain_name> must be followed by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "QUEUE_STATUS")) {
        domain->command().makeQueueStatus();
        return expectEnd(error, next);  // RETURN
    }

    *error = "Invalid subcommand after CLUSTERS CLUSTER <name> STORAGE DOMAIN "
             "<domain_name>: " +
             subcommand;
    return -1;
}

/// CLUSTERS CLUSTER <name> STATE ...
int parseClusterState(ClusterCommand* command,
                      bsl::string*    error,
                      WordGenerator   next)
{
    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "The command CLUSTERS CLUSTER <name> STATE must be followed "
                 "by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "ELECTOR")) {
        return parseElector(&command->makeState().makeElector(),
                            error,
                            next);  // RETURN
    }

    *error = "Invalid subcommand after CLUSTERS CLUSTER <name> STATE: ";
    *error += subcommand;
    return -1;  // RETURN
}

/// CLUSTERS CLUSTER <name> STATE ELECTOR ...
int parseElector(ElectorCommand* command,
                 bsl::string*    error,
                 WordGenerator   next)
{
    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "The command CLUSTERS CLUSTER <name> STATE ELECTOR must be "
                 "followed by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "SET") ||
        equalCaseless(subcommand, "SET_ALL")) {
        const bslstl::StringRef parameter = next();

        if (parameter.empty()) {
            *error = "The command CLUSTERS CLUSTER <name> STATE ELECTOR "
                     "[SET|SET_ALL] "
                     "must be followed by a parameter name.";
            return -1;  // RETURN
        }

        const bslstl::StringRef valueString = next();

        if (valueString.empty()) {
            *error = "The command CLUSTERS CLUSTER <name> STATE ELECTOR "
                     "[SET|SET_ALL] "
                     "<parameter> must be followed by a new value for the "
                     "parameter.";
            return -1;  // RETURN
        }

        SetTunable& tunable = command->makeSetTunable();
        tunable.name()      = parameter;
        tunable.value()     = parseValue(valueString);

        if (equalCaseless(subcommand, "SET")) {
            tunable.choice().makeSelf();
        }
        else {  // SET_ALL
            tunable.choice().makeAll();
        }
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "GET") ||
             equalCaseless(subcommand, "GET_ALL")) {
        const bslstl::StringRef parameter = next();

        if (parameter.empty()) {
            *error = "The command CLUSTERS CLUSTER <name> STATE ELECTOR "
                     "[GET|GET_ALL] "
                     "must be followed by a parameter name.";
            return -1;  // RETURN
        }

        GetTunable& tunable = command->makeGetTunable();
        tunable.name()      = parameter;

        if (equalCaseless(subcommand, "GET")) {
            tunable.choice().makeSelf();
        }
        else {
            tunable.choice().makeAll();
        }

        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "LIST_TUNABLES")) {
        command->makeListTunables();
        return expectEnd(error, next);  // RETURN
    }

    *error = "Invalid subcommand after CLUSTERS CLUSTER <name> STATE "
             "ELECTOR: " +
             subcommand;
    return -1;
}

/// DANGER ...
int parseDanger(DangerCommand* command, bsl::string* error, WordGenerator next)
{
    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "The DANGER command must be followed by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "SHUTDOWN")) {
        command->makeShutdown();
        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "TERMINATE")) {
        command->makeTerminate();
        return expectEnd(error, next);  // RETURN
    }

    *error = "Invalid subcommand after DANGER: " + subcommand;
    return -1;
}

/// CLUSTERS CLUSTER <name> STORAGE REPLICATION ...
int parseReplication(ReplicationCommand* command,
                     bsl::string*        error,
                     WordGenerator       next)
{
    const bslstl::StringRef subcommand = next();

    if (subcommand.empty()) {
        *error = "The command CLUSTERS CLUSTER <name> STORAGE REPLICATION "
                 "must be followed by a subcommand.";
        return -1;  // RETURN
    }

    if (equalCaseless(subcommand, "SET") ||
        equalCaseless(subcommand, "SET_ALL")) {
        const bslstl::StringRef parameter = next();

        if (parameter.empty()) {
            *error = "The command CLUSTERS CLUSTER <name> STORAGE REPLICATION "
                     "SET must be followed by a parameter name.";
            return -1;  // RETURN
        }

        const bslstl::StringRef valueString = next();

        if (valueString.empty()) {
            *error = "The command CLUSTERS CLUSTER <name> STORAGE REPLICATION "
                     "SET <parameter> must be followed by a new value for the "
                     "parameter.";
            return -1;  // RETURN
        }

        SetTunable& tunable = command->makeSetTunable();
        tunable.name()      = parameter;
        tunable.value()     = parseValue(valueString);

        if (equalCaseless(subcommand, "SET")) {
            tunable.choice().makeSelf();
        }
        else {  // SET_ALL
            tunable.choice().makeAll();
        }

        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "GET") ||
             equalCaseless(subcommand, "GET_ALL")) {
        const bslstl::StringRef parameter = next();

        if (parameter.empty()) {
            *error = "The command CLUSTERS CLUSTER <name> STORAGE REPLICATION "
                     "GET must be followed by a parameter name.";
            return -1;  // RETURN
        }

        GetTunable& tunable = command->makeGetTunable();
        tunable.name()      = parameter;

        if (equalCaseless(subcommand, "GET")) {
            tunable.choice().makeSelf();
        }
        else {  // GET_ALL
            tunable.choice().makeAll();
        }

        return expectEnd(error, next);  // RETURN
    }
    else if (equalCaseless(subcommand, "LIST_TUNABLES")) {
        command->makeListTunables();
        return expectEnd(error, next);  // RETURN
    }

    *error = "Invalid subcommand after CLUSTERS CLUSTER <name> STORAGE "
             "REPLICATION: " +
             subcommand;
    return -1;
}

}  // close unnamed namespace

// ----------------
// struct ParseUtil
// ----------------

int ParseUtil::parse(Command*                 command,
                     bsl::string*             error,
                     const bslstl::StringRef& input)
{
    // PRECONDITIONS
    BSLS_ASSERT(command);
    BSLS_ASSERT(error);

    if (input.length() == 0) {
        error->assign("Empty input");
        return -1;  // RETURN
    }

    // Try to decode the input as a JSON string if it starts by '{'
    if (input.data()[0] == '{') {
        bdlsb::FixedMemInStreamBuf jsonStreamBuf(input.data(), input.length());
        baljsn::Decoder            decoder;
        baljsn::DecoderOptions     options;
        options.setSkipUnknownElements(true);

        int rc = decoder.decode(&jsonStreamBuf, command, options);
        if (rc != 0) {
            mwcu::MemOutStream err;
            err << "Error decoding JSON command "
                << "[rc: " << rc << ", error: '" << decoder.loggedMessages()
                << "']";
            error->assign(err.str().data(), err.str().length());
            return -1;  // RETURN
        }

        return 0;  // RETURN
    }

    // Split 'input' into space-separated words, but first collapse any
    // contiguous spaces so that the resulting split doesn't contain any empty
    // strings.
    bsl::string inputString(input);
    mwcu::StringUtil::squeeze(&inputString, " ");

    const bsl::vector<bslstl::StringRef> words =
        mwcu::StringUtil::strTokenizeRef(inputString, " ");

    return parseCommand(command,
                        error,
                        WordGenerator(words.data(),
                                      words.data() + words.size()));
}

}  // close package namespace
}  // close enterprise namespace
