// Copyright 2020-2023 Bloomberg Finance L.P.
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

// mqbcmd_util.cpp                                                    -*-C++-*-
#include <mqbcmd_humanprinter.h>
#include <mqbcmd_jsonprinter.h>
#include <mqbcmd_util.h>

#include <mqbscm_version.h>
// BDE
#include <ball_log.h>
#include <bsl_vector.h>
#include <bsls_assert.h>

namespace BloombergLP {

BALL_LOG_SET_NAMESPACE_CATEGORY("MQBCMD.UTIL");

namespace mqbcmd {

// -----------
// struct Util
// -----------

void Util::flatten(Result* result, const InternalResult& cmdResult)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(result && !cmdResult.isUndefinedValue());

    if (cmdResult.isErrorValue()) {
        result->makeError(cmdResult.error());
        return;  // RETURN
    }
    else if (cmdResult.isSuccessValue()) {
        result->makeSuccess();
        return;  // RETURN
    }
    else if (cmdResult.isHelpValue()) {
        result->makeHelp(cmdResult.help());
        return;  // RETURN
    }
    else if (cmdResult.isDomainsResultValue()) {
        const DomainsResult& domainsResult = cmdResult.domainsResult();
        if (domainsResult.isDomainResultValue()) {
            const DomainResult& domainResult = domainsResult.domainResult();
            if (domainResult.isDomainInfoValue()) {
                result->makeDomainInfo(domainResult.domainInfo());
                return;  // RETURN
            }
            else if (domainResult.isPurgedQueuesValue()) {
                result->makePurgedQueues(domainResult.purgedQueues());
                return;  // RETURN
            }
            else if (domainResult.isQueueResultValue()) {
                const QueueResult& queueResult = domainResult.queueResult();
                if (queueResult.isPurgedQueuesValue()) {
                    result->makePurgedQueues(queueResult.purgedQueues());
                    return;  // RETURN
                }
                else if (queueResult.isQueueContentsValue()) {
                    result->makeQueueContents(queueResult.queueContents());
                    return;  // RETURN
                }
                else if (queueResult.isQueueInternalsValue()) {
                    result->makeQueueInternals(queueResult.queueInternals());
                    return;  // RETURN
                }
            }
        }
    }
    else if (cmdResult.isClustersResultValue()) {
        const ClustersResult& clustersResult = cmdResult.clustersResult();
        if (clustersResult.isClusterListValue()) {
            result->makeClusterList(clustersResult.clusterList());
            return;  // RETURN
        }
        else if (clustersResult.isClusterResultValue()) {
            const ClusterResult& clusterResult =
                clustersResult.clusterResult();
            if (clusterResult.isElectorResultValue()) {
                const ElectorResult& electorResult =
                    clusterResult.electorResult();
                if (electorResult.isTunableValue()) {
                    result->makeTunable(electorResult.tunable());
                    return;  // RETURN
                }
                else if (electorResult.isTunablesValue()) {
                    result->makeTunables(electorResult.tunables());
                    return;  // RETURN
                }
                else if (electorResult.isTunableConfirmationValue()) {
                    result->makeTunableConfirmation(
                        electorResult.tunableConfirmation());
                    return;  // RETURN
                }
            }
            else if (clusterResult.isStorageResultValue()) {
                const StorageResult& storageResult =
                    clusterResult.storageResult();
                if (storageResult.isClusterStorageSummaryValue()) {
                    result->makeClusterStorageSummary(
                        storageResult.clusterStorageSummary());
                    return;  // RETURN
                }
                else if (storageResult.isStorageContentValue()) {
                    result->makeStorageContent(storageResult.storageContent());
                    return;  // RETURN
                }
                else if (storageResult.isReplicationResultValue()) {
                    const ReplicationResult& replicationResult =
                        storageResult.replicationResult();
                    if (replicationResult.isTunableValue()) {
                        result->makeTunable(replicationResult.tunable());
                        return;  // RETURN
                    }
                    else if (replicationResult.isTunablesValue()) {
                        result->makeTunables(replicationResult.tunables());
                        return;  // RETURN
                    }
                    else if (replicationResult.isTunableConfirmationValue()) {
                        result->makeTunableConfirmation(
                            replicationResult.tunableConfirmation());
                        return;  // RETURN
                    }
                }
            }
            else if (clusterResult.isClusterQueueHelperValue()) {
                result->makeClusterQueueHelper(
                    clusterResult.clusterQueueHelper());
                return;  // RETURN
            }
            else if (clusterResult.isClusterStatusValue()) {
                result->makeClusterStatus(clusterResult.clusterStatus());
                return;  // RETURN
            }
            else if (clusterResult.isClusterProxyStatusValue()) {
                result->makeClusterProxyStatus(
                    clusterResult.clusterProxyStatus());
                return;  // RETURN
            }
        }
    }
    else if (cmdResult.isBrokerConfigValue()) {
        result->makeBrokerConfig(cmdResult.brokerConfig());
        return;  // RETURN
    }
    else if (cmdResult.isStatResultValue()) {
        const StatResult& statResult = cmdResult.statResult();

        if (statResult.isStatsValue()) {
            result->makeStats(cmdResult.statResult().stats());
            return;  // RETURN
        }
        else if (statResult.isTunableValue()) {
            result->makeTunable(statResult.tunable());
            return;  // RETURN
        }
        else if (statResult.isTunablesValue()) {
            result->makeTunables(statResult.tunables());
            return;  // RETURN
        }
        else if (statResult.isTunableConfirmationValue()) {
            result->makeTunableConfirmation(statResult.tunableConfirmation());
            return;  // RETURN
        }
    }

    BALL_LOG_ERROR << "Unsupported command result: " << cmdResult;
    BSLS_ASSERT_SAFE(false && "Unsupported result");
}

void Util::printCommandResult(const mqbcmd::InternalResult& cmdResult,
                              mqbcmd::EncodingFormat::Value encoding,
                              bsl::ostream&                 os)
{
    // Flatten into the final result
    mqbcmd::Result result;
    mqbcmd::Util::flatten(&result, cmdResult);

    switch (encoding) {
    case mqbcmd::EncodingFormat::TEXT: {
        // Pretty print
        mqbcmd::HumanPrinter::print(os, result);
    } break;  // BREAK
    case mqbcmd::EncodingFormat::JSON_COMPACT: {
        mqbcmd::JsonPrinter::print(os, result, false);
    } break;  // BREAK
    case mqbcmd::EncodingFormat::JSON_PRETTY: {
        mqbcmd::JsonPrinter::print(os, result, true);
    } break;  // BREAK
    default: BSLS_ASSERT_SAFE(false && "Unsupported encoding");
    }
}

void Util::printCommandResponses(const mqbcmd::RouteResponseList& responseList,
                                 const mqbcmd::EncodingFormat::Value format,
                                 bsl::ostream&                       os)
{
    typedef bsl::vector<BloombergLP::mqbcmd::RouteResponse>
        RouteResponseVector;

    RouteResponseVector responses = responseList.responses();

    // When there is only 1 response (as in single route or self exec.)
    // then just display that result. It should already be formatted properly.
    if (responses.size() == 1) {
        os << responses[0].response();
        return;  // RETURN
    }

    switch (format) {
    case mqbcmd::EncodingFormat::TEXT: {
        mqbcmd::HumanPrinter::printResponses(os, responseList);
    } break;  // BREAK
    case mqbcmd::EncodingFormat::JSON_COMPACT: {
        mqbcmd::JsonPrinter::printResponses(os, responseList, false);
    } break;  // BREAK
    case mqbcmd::EncodingFormat::JSON_PRETTY: {
        mqbcmd::JsonPrinter::printResponses(os, responseList, true);
    } break;  // BREAK
    }
}

}  // close package namespace
}  // close enterprise namespace
