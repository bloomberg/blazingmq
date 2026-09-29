// Copyright 2026 Bloomberg Finance L.P.
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

// End-to-end fuzzer: a single-node broker runs in this process, and each input
// is sent as is over a new client connection.

// MQB
#include <mqba_application.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>

// BMQ
#include <bmqp_blobpoolutil.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqscm_version.h>
#include <bmqst_statcontext.h>
#include <bmqt_queueflags.h>
#include <bmqu_memoutstream.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <ball_loggermanager.h>
#include <ball_loggermanagerconfiguration.h>
#include <ball_severity.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlmt_eventscheduler.h>
#include <bdls_filesystemutil.h>
#include <bdlsb_fixedmeminstreambuf.h>
#include <bsl_cstdio.h>
#include <bsl_cstdlib.h>
#include <bsl_cstring.h>
#include <bsl_fstream.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>
#include <bsls_systemclocktype.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>

// SYSTEM
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace BloombergLP;

namespace {

const int k_READY_REQUEST_ID    = 1;
const int k_STARTUP_TIMEOUT_SEC = 120;

const char k_READY_QUEUE_URI[] = "bmq://bmq.test.mem.priority/fuzz";

/// @brief Print the specified `message` and abort the process.
///
/// @param message The reason of the failure.
BSLA_NORETURN void fail(const char* message)
{
    bsl::fprintf(stderr, "s_bmqfuzz_mqba_application_client: %s\n", message);
    bsl::abort();
}

/// @brief Print the specified `message` with the description of the current
///        `errno` and abort the process.
///
/// @param message The reason of the failure.
BSLA_NORETURN void failWithErrno(const char* message)
{
    bsl::fprintf(stderr,
                 "s_bmqfuzz_mqba_application_client: %s: %s\n",
                 message,
                 bsl::strerror(errno));
    bsl::abort();
}

/// @brief Return a deadline on the monotonic clock.
///
/// @param seconds The number of seconds from now.
///
/// @return The time point `seconds` from now.
bsls::TimeInterval deadlineIn(int seconds)
{
    return bsls::SystemTime::nowMonotonicClock().addSeconds(seconds);
}

/// @brief Find a free TCP port on the loopback interface.
///
/// @return The port number, or a negative value on error.
int findFreePort()
{
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;  // RETURN
    }

    sockaddr_in address;
    bsl::memset(&address, 0, sizeof(address));
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int       port   = -1;
    socklen_t length = sizeof(address);
    if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) ==
            0 &&
        ::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &length) ==
            0) {
        port = ntohs(address.sin_port);
    }
    ::close(fd);
    return port;
}

/// @brief Write the specified `content` to the file at the specified `path`.
///
/// @param path    The file to create or overwrite.
/// @param content The data to write.
///
/// @return 0 on success, or a non-zero value otherwise.
int writeFile(const bsl::string& path, bsl::string_view content)
{
    bsl::ofstream file(path.c_str());
    file << content;
    file.close();
    return file ? 0 : -1;
}

// ============
// class Broker
// ============

/// Single-node broker running in this process, with its configuration
/// stored in a temporary directory.
class Broker {
  private:
    // DATA
    bsl::string                          d_dir;
    int                                  d_port;
    bdlmt::EventScheduler                d_scheduler;
    bslma::ManagedPtr<mqba::Application> d_app_mp;
    bslma::Allocator*                    d_allocator_p;

    // NOT IMPLEMENTED
    Broker(const Broker&) BSLS_KEYWORD_DELETED;
    Broker& operator=(const Broker&) BSLS_KEYWORD_DELETED;

    // PRIVATE MANIPULATORS

    /// @brief Write the cluster and domain configuration files.
    ///
    /// @return 0 on success, or a non-zero value otherwise.
    int writeConfigFiles();

    /// @brief Load the broker configuration.
    ///
    /// @param[out] config           The loaded configuration.
    /// @param[out] errorDescription The description of the error, if any.
    ///
    /// @return 0 on success, or a non-zero value otherwise.
    int loadAppConfig(mqbcfg::AppConfig* config,
                      bsl::ostream&      errorDescription);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Broker, bslma::UsesBslmaAllocator)

    // CREATORS

    /// @brief Create a stopped broker.
    ///
    /// @param allocator The allocator to use.
    explicit Broker(bslma::Allocator* allocator);

    /// @brief Stop the broker and remove its temporary directory.
    ~Broker();

    // MANIPULATORS

    /// @brief Configure and start the broker.
    ///
    /// @param[out] errorDescription The description of the error, if any.
    ///
    /// @return 0 on success, or a non-zero value otherwise.
    int start(bsl::ostream& errorDescription);

    // ACCESSORS

    /// @return The port the broker listens on.
    int port() const;
};

Broker::Broker(bslma::Allocator* allocator)
: d_dir(allocator)
, d_port(-1)
, d_scheduler(bsls::SystemClockType::e_MONOTONIC, allocator)
, d_app_mp()
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_allocator_p);
}

Broker::~Broker()
{
    if (d_app_mp) {
        d_app_mp->stop();
        d_app_mp.reset();
    }
    d_scheduler.stop();
    if (!d_dir.empty()) {
        bdls::FilesystemUtil::remove(d_dir, true);
    }
}

int Broker::writeConfigFiles()
{
    bmqu::MemOutStream clusters(d_allocator_p);
    clusters << R"({
    "myClusters": [
        {
            "name": "local",
            "clusterAttributes": {
                "isCSLModeEnabled": false,
                "isFSMWorkflow": false
            },
            "nodes": [
                {
                    "id": 0,
                    "dataCenter": "UNSPECIFIED",
                    "name": "localhost",
                    "transport": {
                        "tcp": {
                            "endpoint": "tcp://localhost:)"
             << d_port << R"("
                        }
                    }
                }
            ],
            "partitionConfig": {
                "flushAtShutdown": true,
                "location": ")"
             << d_dir << R"(/storage",
                "maxArchivedFileSets": 0,
                "maxDataFileSize": 268435456,
                "maxJournalFileSize": 67108864,
                "maxQlistFileSize": 8388608,
                "maxCSLFileSize": 8388608,
                "numPartitions": 1,
                "preallocate": false,
                "prefaultPages": false,
                "archiveLocation": ")"
             << d_dir << R"(/storage/archive",
                "syncConfig": {
                    "fileChunkSize": 0,
                    "masterSyncMaxDurationMs": 0,
                    "maxAttemptsStorageSync": 0,
                    "partitionSyncDataReqTimeoutMs": 0,
                    "partitionSyncEventSize": 0,
                    "partitionSyncStateReqTimeoutMs": 0,
                    "startupRecoveryMaxDurationMs": 0,
                    "startupWaitDurationMs": 0,
                    "storageSyncReqTimeoutMs": 0
                }
            },
            "masterAssignment": "E_LEADER_IS_MASTER_ALL",
            "elector": {
                "electionResultTimeoutMs": 4000,
                "heartbeatBroadcastPeriodMs": 2000,
                "heartbeatCheckPeriodMs": 1000,
                "heartbeatMissCount": 10,
                "initialWaitTimeoutMs": 8000,
                "leaderSyncDelayMs": 80000,
                "maxRandomWaitTimeoutMs": 3000,
                "quorum": 0
            },
            "queueOperations": {
                "ackWindowSize": 500,
                "assignmentTimeoutMs": 15000,
                "closeTimeoutMs": 300000,
                "configureTimeoutMs": 300000,
                "consumptionMonitorPeriodMs": 30000,
                "keepaliveDurationMs": 1800000,
                "openTimeoutMs": 300000,
                "reopenMaxAttempts": 10,
                "reopenRetryIntervalMs": 5000,
                "reopenTimeoutMs": 43200000,
                "shutdownTimeoutMs": 20000,
                "stopTimeoutMs": 10000
            },
            "clusterMonitorConfig": {
                "maxTimeLeader": 60,
                "maxTimeMaster": 120,
                "maxTimeNode": 120,
                "maxTimeFailover": 240,
                "thresholdLeader": 30,
                "thresholdMaster": 60,
                "thresholdNode": 60,
                "thresholdFailover": 120
            },
            "messageThrottleConfig": {
                "lowThreshold": 2,
                "highThreshold": 4,
                "lowInterval": 1000,
                "highInterval": 3000
            }
        }
    ],
    "proxyClusters": []
})";

    const char domain[] = R"({
    "definition": {
        "location": "local",
        "parameters": {
            "maxDeliveryAttempts": 0,
            "deduplicationTimeMs": 300000,
            "consistency": {
                "eventual": {}
            },
            "storage": {
                "config": {
                    "inMemory": {}
                },
                "domainLimits": {
                    "bytes": 2097152,
                    "messages": 2000,
                    "bytesWatermarkRatio": 0.8,
                    "messagesWatermarkRatio": 0.8
                },
                "queueLimits": {
                    "bytes": 1048576,
                    "messages": 1000,
                    "bytesWatermarkRatio": 0.8,
                    "messagesWatermarkRatio": 0.8
                }
            },
            "messageTtl": 300,
            "maxProducers": 0,
            "maxConsumers": 0,
            "maxQueues": 0,
            "maxIdleTime": 0,
            "mode": {
                "priority": {}
            }
        }
    }
})";

    if (bdls::FilesystemUtil::createDirectories(d_dir + "/domains", true) !=
            0 ||
        bdls::FilesystemUtil::createDirectories(d_dir + "/storage/archive",
                                                true) != 0) {
        return -1;  // RETURN
    }

    if (writeFile(d_dir + "/clusters.json",
                  bsl::string_view(clusters.str().data(),
                                   clusters.str().length())) != 0) {
        return -2;  // RETURN
    }

    return writeFile(d_dir + "/domains/bmq.test.mem.priority.json", domain);
}

int Broker::loadAppConfig(mqbcfg::AppConfig* config,
                          bsl::ostream&      errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);

    bmqu::MemOutStream json(d_allocator_p);
    json << R"({
    "brokerInstanceName": "fuzz",
    "brokerVersion": 999999,
    "configVersion": 999999,
    "etcDir": ")"
         << d_dir << R"(",
    "hostName": "localhost",
    "hostTags": "standalone",
    "hostDataCenter": "UNSPECIFIED",
    "logsObserverMaxSize": 100,
    "dispatcherConfig": {
        "sessions": {
            "numProcessors": 1,
            "processorConfig": {
                "queueSizeLowWatermark": 100000,
                "queueSizeHighWatermark": 200000,
                "queueSize": 500000
            }
        },
        "queues": {
            "numProcessors": 1,
            "processorConfig": {
                "queueSizeLowWatermark": 100000,
                "queueSizeHighWatermark": 200000,
                "queueSize": 500000
            }
        },
        "clusters": {
            "numProcessors": 1,
            "processorConfig": {
                "queueSizeLowWatermark": 100000,
                "queueSizeHighWatermark": 200000,
                "queueSize": 500000
            }
        }
    },
    "stats": {
        "snapshotInterval": 1,
        "printer": {
            "printInterval": 0,
            "file": ")"
         << d_dir << R"(/stat.%T.%p",
            "maxAgeDays": 1
        }
    },
    "networkInterfaces": {
        "heartbeats": {
            "client": 0,
            "downstreamBroker": 0,
            "upstreamBroker": 0,
            "clusterPeer": 0
        },
        "tcpInterface": {
            "name": "TCPInterface",
            "port": )"
         << d_port << R"(,
            "ioThreads": 1,
            "maxConnections": 10000,
            "lowWatermark": 4194304,
            "highWatermark": 1073741824,
            "nodeLowWatermark": 5242880,
            "nodeHighWatermark": 10485760,
            "heartbeatIntervalMs": 3000
        }
    },
    "bmqconfConfig": {
        "cacheTTLSeconds": 30
    }
})";

    baljsn::Decoder        decoder(d_allocator_p);
    baljsn::DecoderOptions options;
    options.setSkipUnknownElements(true);

    bdlsb::FixedMemInStreamBuf streamBuf(json.str().data(),
                                         json.str().length());
    const int rc = decoder.decode(&streamBuf, config, options);
    if (rc != 0) {
        errorDescription << "failed to decode the broker config [rc: " << rc
                         << ", error: " << decoder.loggedMessages() << "]";
    }
    return rc;
}

int Broker::start(bsl::ostream& errorDescription)
{
    const char* tmpDir = bsl::getenv("TMPDIR");
    bsl::string prefix(tmpDir ? tmpDir : "/tmp", d_allocator_p);
    prefix.append("/bmqfuzz_broker_");

    if (bdls::FilesystemUtil::createTemporaryDirectory(&d_dir, prefix) != 0) {
        errorDescription << "failed to create a temporary directory";
        return -1;  // RETURN
    }

    d_port = findFreePort();
    if (d_port < 0) {
        errorDescription << "failed to find a free port";
        return -2;  // RETURN
    }

    if (writeConfigFiles() != 0) {
        errorDescription << "failed to write the config files to " << d_dir;
        return -3;  // RETURN
    }

    mqbcfg::AppConfig config(d_allocator_p);
    if (loadAppConfig(&config, errorDescription) != 0) {
        return -4;  // RETURN
    }
    mqbcfg::BrokerConfig::set(config);

    if (d_scheduler.start() != 0) {
        errorDescription << "failed to start the scheduler";
        return -5;  // RETURN
    }

    d_app_mp = bslma::ManagedPtrUtil::allocateManaged<mqba::Application>(
        d_allocator_p,
        &d_scheduler,
        static_cast<bmqst::StatContext*>(0));

    return d_app_mp->start(errorDescription);
}

int Broker::port() const
{
    return d_port;
}

// ============
// class Client
// ============

/// Blocking TCP client speaking the BlazingMQ protocol.
class Client {
  public:
    // TYPES
    enum Status {
        e_SUCCESS,  ///< The operation completed.
        e_CLOSED,   ///< The broker closed the connection or disconnected.
        e_TIMEOUT   ///< The deadline expired.
    };

  private:
    // DATA
    bdlbb::PooledBlobBufferFactory   d_bufferFactory;
    bmqp::BlobPoolUtil::BlobSpPoolSp d_blobSpPool_sp;
    int                              d_fd;
    bslma::Allocator*                d_allocator_p;

    // NOT IMPLEMENTED
    Client(const Client&) BSLS_KEYWORD_DELETED;
    Client& operator=(const Client&) BSLS_KEYWORD_DELETED;

    // PRIVATE MANIPULATORS

    /// @brief Read exactly the specified `length` bytes.
    ///
    /// @param[out] buffer   The destination of the bytes.
    /// @param      length   The number of bytes to read.
    /// @param      deadline The time point to give up at.
    ///
    /// @return The status of the read.
    Status
    readBytes(char* buffer, size_t length, const bsls::TimeInterval& deadline);

    /// @brief Read one event.
    ///
    /// @param[out] event    The event read.
    /// @param      deadline The time point to give up at.
    ///
    /// @return The status of the read.
    Status readEvent(bdlbb::Blob* event, const bsls::TimeInterval& deadline);

  public:
    // CREATORS

    /// @brief Create a disconnected client.
    ///
    /// @param allocator The allocator to use.
    explicit Client(bslma::Allocator* allocator);

    /// @brief Close the connection, if any.
    ~Client();

    // MANIPULATORS

    /// @brief Connect to the broker on the loopback interface.
    ///
    /// @param port The port of the broker.
    ///
    /// @return 0 on success, or a non-zero value otherwise.
    int connect(int port);

    /// @brief Send the specified bytes.
    ///
    /// @param data   The bytes to send.
    /// @param length The number of bytes to send.
    ///
    /// @return 0 on success, or a non-zero value otherwise.
    int send(const char* data, size_t length);

    /// @brief Send the specified `message` as a control event.
    ///
    /// @param message The message to send.
    ///
    /// @return 0 on success, or a non-zero value otherwise.
    template <class TYPE>
    int sendControlMessage(const TYPE& message);

    /// @brief Negotiate the session as a client.
    ///
    /// @param deadline The time point to give up at.
    ///
    /// @return 0 on success, or a non-zero value otherwise.
    int negotiate(const bsls::TimeInterval& deadline);

    /// @brief Wait for the reply to the request having the specified
    ///        `requestId`, ignoring any other event.
    ///
    /// @param[out] reply     The reply received.
    /// @param      requestId The id of the request.
    /// @param      deadline  The time point to give up at.
    ///
    /// @return `e_SUCCESS` if `reply` was loaded, or the reason otherwise.
    Status waitForReply(bmqp_ctrlmsg::ControlMessage* reply,
                        int                           requestId,
                        const bsls::TimeInterval&     deadline);
};

Client::Client(bslma::Allocator* allocator)
: d_bufferFactory(4096, allocator)
, d_blobSpPool_sp(
      bmqp::BlobPoolUtil::createBlobPool(&d_bufferFactory, allocator))
, d_fd(-1)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_allocator_p);
}

Client::~Client()
{
    if (d_fd >= 0) {
        // Reset the connection on close, so the local port is not held in
        // `TIME_WAIT`.
        linger noLinger;
        noLinger.l_onoff  = 1;
        noLinger.l_linger = 0;
        ::setsockopt(d_fd, SOL_SOCKET, SO_LINGER, &noLinger, sizeof(noLinger));
        ::close(d_fd);
    }
}

Client::Status Client::readBytes(char*                     buffer,
                                 size_t                    length,
                                 const bsls::TimeInterval& deadline)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(buffer);

    while (length > 0) {
        const bsls::TimeInterval remaining =
            deadline - bsls::SystemTime::nowMonotonicClock();
        if (remaining <= bsls::TimeInterval()) {
            return e_TIMEOUT;  // RETURN
        }

        pollfd pfd;
        pfd.fd     = d_fd;
        pfd.events = POLLIN;
        const int rc = ::poll(
            &pfd,
            1,
            static_cast<int>(remaining.totalMilliseconds()) + 1);
        if (rc == 0 || (rc < 0 && errno == EINTR)) {
            continue;  // CONTINUE
        }
        if (rc < 0) {
            return e_CLOSED;  // RETURN
        }

        const ssize_t numRead = ::recv(d_fd, buffer, length, 0);
        if (numRead < 0 && errno == EINTR) {
            continue;  // CONTINUE
        }
        if (numRead <= 0) {
            return e_CLOSED;  // RETURN
        }

        buffer += numRead;
        length -= numRead;
    }

    return e_SUCCESS;
}

Client::Status Client::readEvent(bdlbb::Blob*              event,
                                 const bsls::TimeInterval& deadline)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event);

    bmqp::EventHeader header;
    Status status = readBytes(reinterpret_cast<char*>(&header),
                              sizeof(header),
                              deadline);
    if (status != e_SUCCESS) {
        return status;  // RETURN
    }

    if (header.length() < static_cast<int>(sizeof(header))) {
        fail("the broker sent an event shorter than its header");
    }

    bsl::vector<char> data(header.length(), d_allocator_p);
    bsl::memcpy(data.data(), &header, sizeof(header));
    status = readBytes(data.data() + sizeof(header),
                       data.size() - sizeof(header),
                       deadline);
    if (status != e_SUCCESS) {
        return status;  // RETURN
    }

    event->removeAll();
    bdlbb::BlobUtil::append(event, data.data(), static_cast<int>(data.size()));
    return e_SUCCESS;
}

int Client::connect(int port)
{
    d_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (d_fd < 0) {
        return -1;  // RETURN
    }

    const int noDelay = 1;
    ::setsockopt(d_fd, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));

    sockaddr_in address;
    bsl::memset(&address, 0, sizeof(address));
    address.sin_family      = AF_INET;
    address.sin_port        = htons(static_cast<uint16_t>(port));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    return ::connect(d_fd,
                     reinterpret_cast<sockaddr*>(&address),
                     sizeof(address));
}

int Client::send(const char* data, size_t length)
{
    while (length > 0) {
        const ssize_t numSent = ::send(d_fd, data, length, 0);
        if (numSent < 0 && errno == EINTR) {
            continue;  // CONTINUE
        }
        if (numSent <= 0) {
            return -1;  // RETURN
        }

        data += numSent;
        length -= numSent;
    }

    return 0;
}

template <class TYPE>
int Client::sendControlMessage(const TYPE& message)
{
    bmqp::SchemaEventBuilder builder(d_blobSpPool_sp.get(),
                                     bmqp::EncodingType::e_BER,
                                     d_allocator_p);
    if (builder.setMessage(message, bmqp::EventType::e_CONTROL) != 0) {
        fail("failed to encode a control message");
    }

    const bdlbb::Blob& blob = *builder.blob();
    bsl::vector<char>  data(blob.length(), d_allocator_p);
    bdlbb::BlobUtil::copy(data.data(), blob, 0, blob.length());
    return send(data.data(), data.size());
}

int Client::negotiate(const bsls::TimeInterval& deadline)
{
    bmqp_ctrlmsg::NegotiationMessage request(d_allocator_p);
    bmqp_ctrlmsg::ClientIdentity&    identity = request.makeClientIdentity();
    identity.protocolVersion() = bmqp::Protocol::k_VERSION;
    identity.sdkVersion()      = bmqscm::Version::versionAsInt();
    identity.clientType()      = bmqp_ctrlmsg::ClientType::E_TCPCLIENT;
    identity.processName()     = "s_bmqfuzz";
    identity.sdkLanguage()     = bmqp_ctrlmsg::ClientLanguage::E_CPP;
    identity.features().append(bmqp::EncodingFeature::k_FIELD_NAME)
        .append(":")
        .append(bmqp::EncodingFeature::k_ENCODING_BER);

    if (sendControlMessage(request) != 0) {
        return -1;  // RETURN
    }

    bdlbb::Blob blob(&d_bufferFactory, d_allocator_p);
    if (readEvent(&blob, deadline) != e_SUCCESS) {
        return -2;  // RETURN
    }

    bmqp::Event                      event(&blob, d_allocator_p);
    bmqp_ctrlmsg::NegotiationMessage response(d_allocator_p);
    if (!event.isValid() || !event.isControlEvent() ||
        event.loadControlEvent(&response) != 0 ||
        !response.isBrokerResponseValue() ||
        response.brokerResponse().result().category() !=
            bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        return -3;  // RETURN
    }

    return 0;
}

Client::Status Client::waitForReply(bmqp_ctrlmsg::ControlMessage* reply,
                                    int                           requestId,
                                    const bsls::TimeInterval&     deadline)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(reply);

    bdlbb::Blob blob(&d_bufferFactory, d_allocator_p);
    while (true) {
        const Status status = readEvent(&blob, deadline);
        if (status != e_SUCCESS) {
            return status;  // RETURN
        }

        bmqp::Event event(&blob, d_allocator_p);
        if (!event.isValid() || !event.isControlEvent() ||
            event.loadControlEvent(reply) != 0) {
            continue;  // CONTINUE
        }

        // The broker ignores all requests after a disconnect.
        if (reply->choice().isDisconnectResponseValue()) {
            return e_CLOSED;  // RETURN
        }

        if (!reply->rId().isNull() && reply->rId().value() == requestId) {
            return e_SUCCESS;  // RETURN
        }
    }
}

// ====
// Main
// ====

bslma::ManagedPtr<Broker> s_broker_mp;

/// @brief Stop the broker at process exit.
void stopBroker()
{
    s_broker_mp.reset();
}

/// @brief Open a queue on the broker, to make sure it is fully started.
void waitUntilReady()
{
    bslma::Allocator*        allocator = bslma::Default::allocator();
    const bsls::TimeInterval deadline  = deadlineIn(k_STARTUP_TIMEOUT_SEC);

    Client client(allocator);
    if (client.connect(s_broker_mp->port()) != 0) {
        failWithErrno("failed to connect to the broker");
    }
    if (client.negotiate(deadline) != 0) {
        fail("negotiation with the broker failed");
    }

    bmqp_ctrlmsg::ControlMessage request(allocator);
    request.rId().makeValue(k_READY_REQUEST_ID);
    bmqp_ctrlmsg::QueueHandleParameters& parameters =
        request.choice().makeOpenQueue().handleParameters();
    parameters.uri()        = k_READY_QUEUE_URI;
    parameters.flags()      = bmqt::QueueFlags::e_WRITE;
    parameters.writeCount() = 1;

    bmqp_ctrlmsg::ControlMessage reply(allocator);
    if (client.sendControlMessage(request) != 0 ||
        client.waitForReply(&reply, k_READY_REQUEST_ID, deadline) !=
            Client::e_SUCCESS ||
        !reply.choice().isOpenQueueResponseValue()) {
        fail("failed to open a queue on the broker");
    }
}

}  // close unnamed namespace

extern "C" int LLVMFuzzerInitialize(BSLA_MAYBE_UNUSED int*    argc,
                                    BSLA_MAYBE_UNUSED char*** argv)
{
    ::signal(SIGPIPE, SIG_IGN);

    ball::LoggerManagerConfiguration logConfig;
    logConfig.setDefaultThresholdLevelsIfValid(ball::Severity::e_OFF);
    ball::LoggerManager::initSingleton(logConfig);

    bslma::Allocator* allocator = bslma::Default::allocator();
    s_broker_mp = bslma::ManagedPtrUtil::allocateManaged<Broker>(allocator);

    bmqu::MemOutStream error(allocator);
    if (s_broker_mp->start(error) != 0) {
        bsl::fprintf(stderr,
                     "%.*s\n",
                     static_cast<int>(error.str().length()),
                     error.str().data());
        fail("failed to start the broker");
    }
    bsl::atexit(&stopBroker);

    waitUntilReady();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    Client client(bslma::Default::allocator());
    if (client.connect(s_broker_mp->port()) != 0) {
        failWithErrno("failed to connect to the broker");
    }

    client.send(reinterpret_cast<const char*>(data), size);
    return 0;
}
