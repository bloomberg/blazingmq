// Copyright 2023 Bloomberg Finance L.P.
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

// bmqio_ntcchannelfactory.t.cpp                                      -*-C++-*-
#include <bmqio_ntcchannelfactory.h>

// NTC
#include <ntca_upgradeevent.h>
#include <ntca_upgradeoptions.h>
#include <ntcf_system.h>
#include <ntci_interface.h>
#include <ntci_upgradable.h>
#include <ntsa_distinguishedname.h>

// BDE
#include <ball_filteringobserver.h>
#include <ball_loggermanager.h>
#include <ball_testobserver.h>
#include <balst_stacktraceprintutil.h>
#include <bdlb_nullablevalue.h>
#include <bdlb_stringrefutil.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsl_deque.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslmt_latch.h>
#include <bslmt_threadutil.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// BMQ
#include <bmqtst_testhelper.h>
#include <bmqu_blob.h>
#include <bsl_iostream.h>
#include <bsl_map.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace bmqio;

// IMPLEMENTATION NOTES:
//
// 1) There is no resolution map or reliance on a process-wide mechanism to
// override the results of name resolution. NTC supports object-specific
// name resolution and it is assumed those facilities work correctly and the
// machine is capable of resolving "localhost" to 127.0.0.1.
//
// 2) Connecting to an unresolvable or invalid name (like "localfoohost" or
// "localhost:a") fails asynchronously in NTC.
//
// 3) When immediately cancelling a connection to a valid listening socket,
// NTC can detect that the peer has closed the connection while the accepted
// socket is still in the backlog and avoid announcing that a connection
// has been accepted only for the user to learn that it has already been
// closed. The test that ensures that an immediately-cancelled connection
// does not result in a CHANNEL_UP event has been adjusted to reflect this.
//
// 4) NTC does not report failures of individual connection retry attempts,
// only the failure of the overall connection operation.
// ========================================================================
//                       STANDARD BDE ASSERT TEST MACRO
// ------------------------------------------------------------------------

// FUNCTIONS

/// Return `true` if the specified `messageSubstring` is a substring of the
/// message of the specified `record`.
static bool ballFilter(const bsl::string&  messageSubstring,
                       const ball::Record& record,
                       BSLA_UNUSED const ball::Context& context)
{
    return !bdlb::StringRefUtil::strstr(record.fixedFields().messageRef(),
                                        messageSubstring)
                .isEmpty();
}

namespace {

// ==========================
// class Tester_UpgradeCbInfo
// ==========================

/// Arguments used in a call to a `NtcChannelFactory::onUpgrade` handler.
struct UpgradeCbInfo {
    // DATA
    bsl::shared_ptr<ntci::Upgradable> d_upgradable;
    ntca::UpgradeEvent                d_event;

    typedef bsl::allocator<> allocator_type;

    // CREATORS
    UpgradeCbInfo(allocator_type allocator)
    : d_upgradable()
    , d_event(bslma::AllocatorUtil::adapt(allocator))
    {
        // NOTHING
    }

    UpgradeCbInfo(BSLA_MAYBE_UNUSED const UpgradeCbInfo& original,
                  allocator_type                         allocator)
    : d_upgradable()
    , d_event(bslma::AllocatorUtil::adapt(allocator))
    {
        // NOTHING
    }
};

///
class ChannelUpgradeHandler {
  public:
    typedef bsl::map<int, UpgradeCbInfo> ChannelMap;
    typedef bsl::allocator<>             allocator_type;

  private:
    mutable bslmt::Mutex d_mutex;
    ChannelMap           d_channelMap;

  public:
    explicit ChannelUpgradeHandler(allocator_type allocator)
    : d_mutex()
    , d_channelMap(allocator)
    {
    }

    /// Record the upgrade event and associate it with a channel based on its
    /// ID.
    void onUpgrade(const bsl::shared_ptr<bmqio::NtcChannel>& channel,
                   const bsl::shared_ptr<ntci::Upgradable>&  upgradable,
                   const ntca::UpgradeEvent&                 event)
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);

        BMQTST_ASSERT_D("Channel was attempted to upgrade multiple times",
                        !d_channelMap.contains(channel->channelId()));

        UpgradeCbInfo& upgradeInfo = d_channelMap[channel->channelId()];
        upgradeInfo.d_upgradable   = upgradable;
        upgradeInfo.d_event        = event;
    }

    /// Check that each upgrade was successful. Otherwise, fail the test.
    void checkAll() const
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);

        bsl::for_each(d_channelMap.cbegin(),
                      d_channelMap.cend(),
                      ChannelUpgradeHandler::checkChannelMapEntry);
    }

    /// Return the number of upgrade events
    bsl::size_t upgradeCounts() const { return d_channelMap.size(); }

    /// Create a callback function that references this handler
    bsl::function<void(const bsl::shared_ptr<bmqio::NtcChannel>&,
                       const bsl::shared_ptr<ntci::Upgradable>&,
                       const ntca::UpgradeEvent&)>
    makeOnUpgradeCb()
    {
        return bdlf::BindUtil::bindS(
            bslma::AllocatorUtil::adapt(d_channelMap.get_allocator()),
            &ChannelUpgradeHandler::onUpgrade,
            this,
            bdlf::PlaceHolders::_1,
            bdlf::PlaceHolders::_2,
            bdlf::PlaceHolders::_3);
    }

  private:
    /// Check if the upgrade event contained in `value` was completed,
    /// otherwise fail the test.
    static void checkChannelMapEntry(const ChannelMap::value_type& value)
    {
        BMQTST_ASSERT_D("Channel upgrade failed",
                        value.second.d_event.isComplete());
    }
};

}

// CONSTANTS
static const bslstl::StringRef k_BALL_OBSERVER_NAME = "testDriverObserver";
static const ChannelFactoryEvent::Enum CFE_CHANNEL_UP =
    ChannelFactoryEvent::e_CHANNEL_UP;
static const StatusCategory::Enum CAT_SUCCESS = StatusCategory::e_SUCCESS;
static const StatusCategory::Enum CAT_GENERIC =
    StatusCategory::e_GENERIC_ERROR;
static const StatusCategory::Enum CAT_LIMIT = StatusCategory::e_LIMIT;

static const ChannelWatermarkType::Enum WAT_HIGH =
    ChannelWatermarkType::e_HIGH_WATERMARK;
static const ChannelWatermarkType::Enum WAT_LOW =
    ChannelWatermarkType::e_LOW_WATERMARK;

// Adjust attempt interval depending on the platform
#ifdef BSLS_PLATFORM_OS_SOLARIS
static const int k_ATTEMPT_INTERVAL_MS = 300;
#elif defined(                                                                \
    __has_feature)  // Clang-supported method for checking sanitizers.
static const int k_ATTEMPT_INTERVAL_MS =
    (__has_feature(memory_sanitizer) || __has_feature(thread_sanitizer) ||
     __has_feature(undefined_behavior_sanitizer))
        ? 400
        : 1;
#elif defined(__SANITIZE_MEMORY__) || defined(__SANITIZE_THREAD__) ||         \
    defined(__SANITIZE_UNDEFINED__)
// GCC-supported macros for checking MSAN, TSAN and UBSAN.
static const int k_ATTEMPT_INTERVAL_MS = 400;
#else
static const int k_ATTEMPT_INTERVAL_MS = 1;
#endif

// ========================
// class Tester_ChannelInfo
// ========================

/// Information about a Channel stored by a `Tester`.
struct Tester_ChannelInfo {
    // DATA
    bsl::shared_ptr<Channel>               d_channel;
    bsl::deque<ChannelWatermarkType::Enum> d_watermarkEvents;
    bdlbb::Blob                            d_readData;
    bslmt::Mutex                           d_blockMutex;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Tester_ChannelInfo,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    Tester_ChannelInfo(bslma::Allocator* basicAllocator = 0)
    : d_channel()
    , d_watermarkEvents(basicAllocator)
    , d_readData(basicAllocator)
    , d_blockMutex()
    {
        // NOTHING
    }

    Tester_ChannelInfo(const Tester_ChannelInfo& original,
                       bslma::Allocator*         basicAllocator = 0)
    : d_channel(original.d_channel)
    , d_watermarkEvents(original.d_watermarkEvents, basicAllocator)
    , d_readData(original.d_readData, basicAllocator)
    , d_blockMutex()
    {
        // NOTHING
    }
};

// =========================
// class Tester_ResultCbInfo
// =========================

/// Arguments used in a call to a ChannelFactory::ResultCallback.
struct Tester_ResultCbInfo {
    // DATA
    ChannelFactoryEvent::Enum d_event;
    Status                    d_status;
    bsl::shared_ptr<Channel>  d_channel;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Tester_ResultCbInfo,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    Tester_ResultCbInfo(bslma::Allocator* basicAllocator = 0)
    : d_event()
    , d_status(basicAllocator)
    , d_channel()
    {
        // NOTHING
    }

    Tester_ResultCbInfo(const Tester_ResultCbInfo& original,
                        bslma::Allocator*          basicAllocator = 0)
    : d_event(original.d_event)
    , d_status(original.d_status, basicAllocator)
    , d_channel(original.d_channel)
    {
        // NOTHING
    }
};

// =======================
// class Tester_HandleInfo
// =======================

/// Information about a connect or listen Handle stored by a `Tester`.
struct Tester_HandleInfo {
    // DATA
    bsl::shared_ptr<ChannelFactory::OpHandle> d_handle;
    bsl::deque<Tester_ResultCbInfo>           d_resultCbCalls;
    int                                       d_listenPort;
    bsl::deque<UpgradeCbInfo>                 d_upgradeCbCalls;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Tester_HandleInfo,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    Tester_HandleInfo(bslma::Allocator* basicAllocator = 0)
    : d_handle()
    , d_resultCbCalls(basicAllocator)
    , d_listenPort(-1)
    , d_upgradeCbCalls(basicAllocator)
    {
        // NOTHING
    }

    Tester_HandleInfo(const Tester_HandleInfo& original,
                      bslma::Allocator*        basicAllocator = 0)
    : d_handle(original.d_handle)
    , d_resultCbCalls(original.d_resultCbCalls, basicAllocator)
    , d_listenPort(original.d_listenPort)
    , d_upgradeCbCalls(original.d_upgradeCbCalls, basicAllocator)
    {
        // NOTHING
    }
};

// ===========================
// class Tester_ChannelVisitor
// ===========================

struct Tester_ChannelVisitor {
    // DATA
    int d_numChannels;

    // CREATORS
    Tester_ChannelVisitor()
    : d_numChannels(0)
    {
    }

    // MANIPULATORS
    void operator()(const bsl::shared_ptr<NtcChannel>&) { ++d_numChannels; }
};

// ============
// class Tester
// ============

/// Helper class testing a NtcChannelFactory, wrapping its creation and
/// providing convenient wrappers for calling its functions and checking
/// its results.
///
/// Many of the functions take a `int line` argument, which is always
/// passed `L_` and is used in assertion messages to find where an error
/// occurred.
class Tester {
    // PRIVATE TYPES
    typedef Tester_ChannelInfo                 ChannelInfo;
    typedef Tester_HandleInfo                  HandleInfo;
    typedef Tester_ResultCbInfo                ResultCbInfo;
    typedef bsl::map<bsl::string, ChannelInfo> ChannelMap;
    typedef bsl::map<bsl::string, HandleInfo>  HandleMap;
    typedef bsl::shared_ptr<NtcChannel>        NtcChannelPtr;
    typedef bsl::deque<NtcChannelPtr>          PreCreateCbCallList;

  private:
    // DATA
    bslma::Allocator*                            d_allocator_p;
    bdlbb::PooledBlobBufferFactory               d_blobBufferFactory;
    bsl::shared_ptr<ball::TestObserver>          d_ballObserver;
    ChannelMap                                   d_channelMap;
    HandleMap                                    d_handleMap;
    PreCreateCbCallList                          d_preCreateCbCalls;
    bool                                         d_setPreCreateCb;
    bool                                         d_setEncryptionServer;
    bool                                         d_setEncryptionClient;
    bslma::ManagedPtr<NtcChannelFactory>         d_object;
    bslmt::Mutex                                 d_mutex;
    bsl::shared_ptr<ntci::Interface>             d_interface;
    bsl::shared_ptr<ntci::EncryptionCertificate> d_authorityCertificate;
    bsl::shared_ptr<ntci::EncryptionKey>         d_authorityPrivateKey;
    bsl::shared_ptr<ntci::EncryptionCertificate> d_serverCertificate;
    bsl::shared_ptr<ntci::EncryptionKey>         d_serverPrivateKey;
    bsl::shared_ptr<ntci::EncryptionClient>      d_encryptionClient;

    // PRIVATE MANIPULATORS

    /// ResultCb passed to `connect` and `listen`.
    void resultCb(const bsl::string&              handleName,
                  ChannelFactoryEvent::Enum       event,
                  const Status&                   status,
                  const bsl::shared_ptr<Channel>& channel);

    /// Push an upgrade event onto the list of observed upgrade events
    void recordUpgrade(const bsl::shared_ptr<bmqio::NtcChannel>& channel,
                       const bsl::shared_ptr<ntci::Upgradable>&  upgradable,
                       const ntca::UpgradeEvent&                 event);

    /// `channelPreCreationCb` passed to the NtcChannelFactory, if we're
    /// asked to pass one
    void
    preCreationCb(const bsl::shared_ptr<NtcChannel>&               channel,
                  const bsl::shared_ptr<ChannelFactory::OpHandle>& handle);

    /// Copy out all data from the specified `blob` into the storage
    /// associated with the specified `channelName` and return indicating
    /// that we want to read 1 more byte.
    void channelReadCb(const bsl::string& channelName,
                       const Status&      status,
                       int*               numNeeded,
                       bdlbb::Blob*       blob);

    /// Callback invoked when the channel with the specified `channelName`
    /// is closed.
    void channelCloseCb(const bsl::string& channelName);

    /// Callback invoked when the channel with the specified `channelName`
    /// generates a watermark event with the specified `watermarkType`.
    void channelWatermarkCb(const bsl::string&         channelName,
                            ChannelWatermarkType::Enum watermarkType);

    /// Destroy the object being tested and reset all supporting objects.
    void destroy();

    /// Initialize the root authority certificate for the server and client.
    void initEncryptionAuthority();

    /// Initialize the server certificate and private key and configure the
    /// channel factory to use it for upgrading listeners.
    void initEncryptionServer();

    /// Initialize the client encryption settings and configure the
    /// channel factory to use it for upgrading connections.
    void initEncryptionClient();

    // NOT IMPLEMENTED
    Tester(const Tester&);
    Tester& operator=(const Tester&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Tester, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit Tester(bslma::Allocator* basicAllocator = 0);
    ~Tester();

    // MANIPULATORS

    /// Set whether a `preCreationCb` will be set on the `NtcChannelFactory`
    /// the next time `init` is called to the specified `value`.  By default
    /// this is `false`.
    void setPreCreateCb(bool value);

    /// Set whether an `encryptionServer` will be set on the
    /// `NtcChannelFactory` the next time `init` is called to the specified
    /// `value`.  By default this is `false`.
    void setEncryptionServer(bool value);

    /// Set whether an `encryptionClient` will be set on the
    /// `NtcChannelFactory` the next time `init` is called to the specified
    /// `value`.  By default this is `false`.
    void setEncryptionClient(bool value);

    /// Find and return a free port by opening and dropping a handle.
    /// Note that the found port might become occupied in an unlikely event of
    /// another application using it after the handle was dropped.
    int findFreeEphemeralPort();

    /// (Re-)create the object being tested and reset the state of any
    /// supporting objects.
    void init(int line);

    /// Listen for connections at the specified `endpoint` and assign the
    /// returned handle to the specified `handleName`, verifying that the
    /// status of the operation has the specified `resultStatus`.
    void listen(int                      line,
                const bslstl::StringRef& handleName,
                const bslstl::StringRef& endpoint,
                StatusCategory::Enum resultStatus = StatusCategory::e_SUCCESS);

    /// Connect to the specified `endpointOrServer` and assign the
    /// resulting handle to the specified `handleName`, verifying that the
    /// result `status` of the `connect` has the specified `resultStatus`.
    /// If the `endpointOrServer` is a `handleName` used in a previous
    /// successful `listen`, the connect will be made to that port.
    /// Otherwise, the connect will use the `endpointOrServer` as the
    /// endpoint of the connect as-is.  The optionally specified `options`
    /// will be used as the base of the options passed to the `connect`.
    void
    connect(int                          line,
            const bslstl::StringRef&     handleName,
            const bslstl::StringRef&     endpointOrServer,
            const bmqio::ConnectOptions& options,
            StatusCategory::Enum resultStatus = StatusCategory::e_SUCCESS);
    void
    connect(int                      line,
            const bslstl::StringRef& handleName,
            const bslstl::StringRef& endpointOrServer,
            StatusCategory::Enum     resultStatus = StatusCategory::e_SUCCESS);

    /// Call `visitChannels` and verify that the visitor was invoked with
    /// the specified `numChannels` channels.
    void callVisitChannels(int line, int numChannels);

    /// Cancel the handle with the specified `handleName`.
    void cancelHandle(int line, const bslstl::StringRef& handleName);

    /// Close the channel associated with the specified `channelName`.
    void closeChannel(int line, const bslstl::StringRef& channelName);

    /// Cause io operations to the channel with the specified `channelName`
    /// to block until `unblockChannelIo` is called.
    void blockChannelIo(int line, const bslstl::StringRef& channelName);

    /// Unblock io operations for the channel with the specified
    /// `channelName` that were previously blocked by a call to
    /// `blockChannelIo`.
    void unblockChannelIo(int line, const bslstl::StringRef& channelName);

    /// Write the specified `data` to the channel with the specified
    /// `channelName` passing it the specified `highWatermark`, and verify
    /// that the resulting status has the specified `statusCategory`.
    void writeChannel(int                      line,
                      const bslstl::StringRef& channelName,
                      const bslstl::StringRef& data,
                      bsls::Types::Int64       highWatermark  = 2048,
                      StatusCategory::Enum     statusCategory = CAT_SUCCESS);

    /// Read as many bytes as as the length of the specified `data` from
    /// the channel with the specified `channelName` and verify that it
    /// matches the `data`.
    void readChannel(int                      line,
                     const bslstl::StringRef& channelName,
                     const bslstl::StringRef& data);

    /// Check that there's no unread data for the channel with the
    /// specified `channelName`. This function will wait a few ms to give
    /// any recently-written data to be read.
    void checkNoRead(int line, const bslstl::StringRef& channelName);

    /// Check the oldest unchecked call to the ResultCallback
    /// associated with the handle with the specified `handleName`,
    /// checking that it was passed the specified `event`, and the status
    /// has the specified `statusCategory`, and if the `event` is
    /// `e_CHANNEL_UP`, assign the channel with the specified
    /// `channelName`.  This function will wait up to 5s for the call to be
    /// received before failing the check.
    void checkResultCallback(
        int                       line,
        const bslstl::StringRef&  handleName,
        ChannelFactoryEvent::Enum event,
        StatusCategory::Enum      statusCategory,
        const bslstl::StringRef&  channelName = bslstl::StringRef());

    /// Check the oldest unchecked call to the ResultCallback associated
    /// with the handle with the specified `handleName`, and verify that
    /// it's a successful `e_CHANNEL_UP` event, and assign the channel the
    /// specified `channelName`.  This function will wait for up to 5s for
    /// the call to be received before failing the check.
    void checkResultCallback(int                      line,
                             const bslstl::StringRef& handleName,
                             const bslstl::StringRef& channelName);

    /// Make sure there are no unchecked calls to the ResultCb associated
    /// with the specified `handleName`.  This function will wait for a
    /// few ms before doing the check.
    void checkNoResultCallback(int line, const bslstl::StringRef& handleName);

    /// Check whether the channel with the specified `channelName` has been
    /// closed or not (depending on the specified `closed`).  This function
    /// will wait for up to 5s for the call to be received before failing
    /// the check.
    void checkChannelClose(int                      line,
                           const bslstl::StringRef& channelName,
                           bool                     closed = true);

    /// Check that the channel with the specified `channelName` received a
    /// `watermark event with the specified `watermarkType'.  This function
    /// will wait for up to 5s for the call to be received before failing
    /// the check.
    void checkChannelWatermark(int                        line,
                               const bslstl::StringRef&   channelName,
                               ChannelWatermarkType::Enum watermarkType);

    /// Check that the channel with the specified `channelName` didn't
    /// receive any watermark events.
    void checkNoChannelWatermark(int                      line,
                                 const bslstl::StringRef& channelName);

    /// Check that the `peerUri` of the channel with the specified
    /// `channelName` starts with the specified `prefix`.
    void checkChannelUri(int                      line,
                         const bslstl::StringRef& channelName,
                         const bslstl::StringRef& prefix);

    /// Start observing BALL records, and remember how many we see that
    /// have the specified `messageSubstring` as a substring of their
    /// message until the next call to `checkFilteredLogs`.
    void startFilteringLogs(const bsl::string& messageSubstring);

    /// Check that the number of BALL records that passed the filter set up
    /// by the last call to `startFilteringLogs` matches the specified
    /// `expected`.
    void checkFilteredLogs(int line, int expected);

    /// Check that the number of calls to the `preCreateCb` matches the
    /// specified `expected`.
    void checkNumPreCreateCbCalls(int line, int expected);

    /// Return a reference providing modifiable access to the object being
    /// tested.
    NtcChannelFactory& object();
};

// ------------
// class Tester
// ------------

// PRIVATE MANIPULATORS
void Tester::resultCb(const bsl::string&              handleName,
                      ChannelFactoryEvent::Enum       event,
                      const Status&                   status,
                      const bsl::shared_ptr<Channel>& channel)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    HandleInfo&                    info = d_handleMap[handleName];

    info.d_resultCbCalls.emplace_back();
    ResultCbInfo& cbInfo = info.d_resultCbCalls.back();
    cbInfo.d_event       = event;
    cbInfo.d_status      = status;
    cbInfo.d_channel     = channel;
}

void Tester::preCreationCb(
    const bsl::shared_ptr<NtcChannel>& channel,
    BSLA_UNUSED const bsl::shared_ptr<ChannelFactory::OpHandle>& handle)
{
    d_preCreateCbCalls.push_back(channel);
}

void Tester::channelReadCb(const bsl::string& channelName,
                           const Status&      status,
                           int*               numNeeded,
                           bdlbb::Blob*       blob)
{
    if (!status) {
        if (status.category() != StatusCategory::e_CANCELED &&
            status.category() != StatusCategory::e_CONNECTION) {
            BMQTST_ASSERT_D(channelName.c_str() << ", " << status.category(),
                            false);
            return;  // RETURN
        }
        return;  // RETURN
    }
    *numNeeded = 1;

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

        bdlbb::Blob& readBlob = d_channelMap[channelName].d_readData;
        bdlbb::BlobUtil::append(&readBlob, *blob);
        blob->removeAll();
    }
    // Acquire the block mutex, blocking us here until it's released
    bslmt::LockGuard<bslmt::Mutex> blockGuard(
        &d_channelMap[channelName].d_blockMutex);  // LOCK
}

void Tester::channelCloseCb(const bsl::string& channelName)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_channelMap[channelName].d_channel.reset();
}

void Tester::channelWatermarkCb(const bsl::string&         channelName,
                                ChannelWatermarkType::Enum watermarkType)
{
    PV("*** " << channelName << " " << watermarkType << ": "
              << " watermark callback");
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    d_channelMap[channelName].d_watermarkEvents.emplace_back(watermarkType);
}

void Tester::destroy()
{
    d_serverPrivateKey.reset();
    d_serverCertificate.reset();
    d_authorityPrivateKey.reset();
    d_authorityCertificate.reset();

    d_interface.reset();
    d_preCreateCbCalls.clear();

    if (d_object) {
        d_object->stop();
    }

    d_ballObserver.clear();
    ball::LoggerManager::singleton().deregisterObserver(k_BALL_OBSERVER_NAME);

    d_handleMap.clear();
    d_channelMap.clear();

    d_object.reset();
}

// CREATORS
Tester::Tester(bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_blobBufferFactory(0xFFFF, basicAllocator)
, d_ballObserver()
, d_channelMap(basicAllocator)
, d_handleMap(basicAllocator)
, d_preCreateCbCalls(basicAllocator)
, d_setPreCreateCb(false)
, d_setEncryptionServer(false)
, d_setEncryptionClient(false)
, d_object()
, d_mutex()
, d_interface()
, d_authorityCertificate()
, d_authorityPrivateKey()
, d_serverCertificate()
, d_serverPrivateKey()
{
}

Tester::~Tester()
{
    destroy();
}

// MANIPULATORS
void Tester::setPreCreateCb(bool value)
{
    d_setPreCreateCb = value;
}

void Tester::setEncryptionServer(bool value)
{
    d_setEncryptionServer = value;
}

void Tester::setEncryptionClient(bool value)
{
    d_setEncryptionClient = value;
}

int Tester::findFreeEphemeralPort()
{
    const char* k_HANDLE_NAME = "listenHandle";
    listen(L_, k_HANDLE_NAME, "127.0.0.1:0", CAT_SUCCESS);

    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    HandleInfo& info = d_handleMap[k_HANDLE_NAME];
    const int   port = info.d_listenPort;

    lock.release()->unlock();

    cancelHandle(L_, k_HANDLE_NAME);
    bslmt::ThreadUtil::yield();

    return port;
}

void Tester::initEncryptionAuthority()
{
    if (d_authorityCertificate && d_authorityPrivateKey) {
        // Authority root has already been initialized, no need to set it again
        return;
    }

    bsl::shared_ptr<ntci::EncryptionCertificate> authorityCertificate;
    bsl::shared_ptr<ntci::EncryptionKey>         authorityPrivateKey;

    // Generate the certificate and private key of a trusted authority.

    ntsa::DistinguishedName authorityIdentity(d_allocator_p);
    authorityIdentity["CN"] = "Authority";
    authorityIdentity["O"]  = "Bloomberg LP";

    ntsa::Error error = d_interface->generateKey(&authorityPrivateKey,
                                                 ntca::EncryptionKeyOptions(),
                                                 d_allocator_p);
    BMQTST_ASSERT(!error);

    ntca::EncryptionCertificateOptions authorityCertificateOptions;
    authorityCertificateOptions.setAuthority(true);

    error = d_interface->generateCertificate(&authorityCertificate,
                                             authorityIdentity,
                                             authorityPrivateKey,
                                             authorityCertificateOptions,
                                             d_allocator_p);
    BMQTST_ASSERT(!error);

    d_authorityCertificate.swap(authorityCertificate);
    d_authorityPrivateKey.swap(authorityPrivateKey);
}

void Tester::initEncryptionServer()
{
    if (d_serverCertificate && d_serverPrivateKey) {
        // Authority root has already been initialized, no need to set it again
        return;
    }

    // Ensure the authority is initialized as its our root of trust
    initEncryptionAuthority();

    // Create an encryption server and configure the encryption server to
    // accept upgrades to TLS 1.3 and higher, to cryptographically identify
    // itself using the server certificate previously generated, to encrypt
    // data using the server private key previously generated, and to not
    // require identification from the client.
    bsl::shared_ptr<ntci::EncryptionCertificate> serverCertificate;
    bsl::shared_ptr<ntci::EncryptionKey>         serverPrivateKey;

    // Generate the certificate and private key of the server, signed
    // by the trusted authority.

    ntsa::DistinguishedName serverIdentity(d_allocator_p);
    serverIdentity["CN"] = "Server";
    serverIdentity["O"]  = "Bloomberg LP";

    ntsa::Error error = d_interface->generateKey(&serverPrivateKey,
                                                 ntca::EncryptionKeyOptions(),
                                                 d_allocator_p);
    BMQTST_ASSERT(!error);

    error = d_interface->generateCertificate(
        &serverCertificate,
        serverIdentity,
        serverPrivateKey,
        d_authorityCertificate,
        d_authorityPrivateKey,
        ntca::EncryptionCertificateOptions(),
        d_allocator_p);
    BMQTST_ASSERT(!error);

    ntca::EncryptionServerOptions encryptionServerOptions;
    encryptionServerOptions.setMinMethod(ntca::EncryptionMethod::e_TLS_V1_3);
    encryptionServerOptions.setMaxMethod(ntca::EncryptionMethod::e_DEFAULT);
    encryptionServerOptions.setAuthentication(
        ntca::EncryptionAuthentication::e_NONE);

    {
        bsl::vector<char> identityData(d_allocator_p);
        serverCertificate->encode(&identityData);
        encryptionServerOptions.setIdentityData(identityData);
    }

    {
        bsl::vector<char> privateKeyData(d_allocator_p);
        serverPrivateKey->encode(&privateKeyData);
        encryptionServerOptions.setPrivateKeyData(privateKeyData);
    }

    // Let the channel factory use the certificate for upgrading incoming
    // connections
    error = object().configureEncryptionServer(encryptionServerOptions);

    d_serverCertificate.swap(serverCertificate);
    d_serverPrivateKey.swap(serverPrivateKey);

    BMQTST_ASSERT(!error);
}

void Tester::initEncryptionClient()
{
    ntsa::Error error;

    initEncryptionAuthority();

    // Create an encryption client and configure the encryption client to
    // request upgrades using TLS 1.2 require identification from the
    // server, and to trust the certificate authority previously generated
    // to verify the authenticity of the server.
    ntca::EncryptionClientOptions encryptionClientOptions;
    encryptionClientOptions.setMinMethod(ntca::EncryptionMethod::e_TLS_V1_3);
    encryptionClientOptions.setMaxMethod(ntca::EncryptionMethod::e_DEFAULT);
    encryptionClientOptions.setAuthentication(
        ntca::EncryptionAuthentication::e_VERIFY);

    {
        bsl::vector<char> authorityData(d_allocator_p);
        d_authorityCertificate->encode(&authorityData);
        encryptionClientOptions.addAuthorityData(authorityData);
    }

    // Let the channel factory use the certificate for upgrading outgoing
    // connections
    error = object().configureEncryptionClient(encryptionClientOptions);

    BMQTST_ASSERT(!error);
}

void Tester::init(int line)
{
    destroy();

    ntca::InterfaceConfig interfaceConfig;
    interfaceConfig.setThreadName("test");

    bsl::shared_ptr<bdlbb::BlobBufferFactory> blobBufferFactory_sp(
        &d_blobBufferFactory,
        bslstl::SharedPtrNilDeleter(),
        d_allocator_p);

    d_interface = ntcf::System::createInterface(interfaceConfig,
                                                blobBufferFactory_sp,
                                                d_allocator_p);

    d_object.load(new (*d_allocator_p)
                      NtcChannelFactory(d_interface, d_allocator_p),
                  d_allocator_p);

    if (d_setPreCreateCb) {
        d_object->onCreate(bdlf::BindUtil::bind(&Tester::preCreationCb,
                                                this,
                                                bdlf::PlaceHolders::_1,
                                                bdlf::PlaceHolders::_2));
    }
    if (d_setEncryptionServer) {
        initEncryptionServer();
    }
    if (d_setEncryptionClient) {
        initEncryptionClient();
    }

    int ret = d_object->start();
    BMQTST_ASSERT_EQ_D(line, ret, 0);
}

void Tester::listen(int                      line,
                    const bslstl::StringRef& handleName,
                    const bslstl::StringRef& endpoint,
                    StatusCategory::Enum     resultStatus)
{
    bsl::string handleNameStr(handleName);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    HandleInfo& info = d_handleMap[handleNameStr];

    ListenOptions options(d_allocator_p);
    options.setEndpoint(endpoint);

    bslma::ManagedPtr<ChannelFactory::OpHandle> opHandle;

    ChannelFactory::ResultCallback resultCb(
        bsl::allocator_arg_t(),
        d_allocator_p,
        bdlf::BindUtil::bindS(d_allocator_p,
                              &Tester::resultCb,
                              this,
                              handleNameStr,
                              bdlf::PlaceHolders::_1,
                              bdlf::PlaceHolders::_2,
                              bdlf::PlaceHolders::_3));

    Status status;
    d_object->listen(&status, &opHandle, options, resultCb);
    info.d_handle = opHandle;

    if (status) {
        info.d_handle->properties().load(
            &info.d_listenPort,
            NtcChannelFactoryUtil::listenPortProperty());
    }
    else {
        d_handleMap.erase(handleNameStr);
    }

    BMQTST_ASSERT_EQ_D(line << ", " << status,
                       status.category(),
                       resultStatus);
}

void Tester::connect(int                          line,
                     const bslstl::StringRef&     handleName,
                     const bslstl::StringRef&     endpointOrServer,
                     const bmqio::ConnectOptions& options,
                     StatusCategory::Enum         resultStatus)
{
    bsl::string handleNameStr(handleName, bmqtst::TestHelperUtil::allocator());

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    HandleInfo& info = d_handleMap[handleNameStr];

    ConnectOptions reqOptions(options, bmqtst::TestHelperUtil::allocator());

    reqOptions.setAttemptInterval(bsls::TimeInterval(
        0,
        k_ATTEMPT_INTERVAL_MS * bdlt::TimeUnitRatio::k_NS_PER_MS));

    HandleMap::iterator serverIter = d_handleMap.find(endpointOrServer);
    if (serverIter != d_handleMap.end()) {
        bsl::ostringstream ss;
        ss << "localhost:" << serverIter->second.d_listenPort;
        reqOptions.setEndpoint(ss.str());
    }
    else {
        reqOptions.setEndpoint(endpointOrServer);
    }

    bslma::ManagedPtr<ChannelFactory::OpHandle> opHandle;

    ChannelFactory::ResultCallback resultCb(
        bsl::allocator_arg_t(),
        d_allocator_p,
        bdlf::BindUtil::bindS(d_allocator_p,
                              &Tester::resultCb,
                              this,
                              handleNameStr,
                              bdlf::PlaceHolders::_1,
                              bdlf::PlaceHolders::_2,
                              bdlf::PlaceHolders::_3));

    Status status;
    d_object->connect(&status, &opHandle, reqOptions, resultCb);
    info.d_handle = opHandle;

    if (!status) {
        d_handleMap.erase(handleNameStr);
    }

    BMQTST_ASSERT_EQ_D(line, status.category(), resultStatus);
}

void Tester::connect(int                      line,
                     const bslstl::StringRef& handleName,
                     const bslstl::StringRef& endpointOrServer,
                     StatusCategory::Enum     resultStatus)
{
    connect(line,
            handleName,
            endpointOrServer,
            bmqio::ConnectOptions(),
            resultStatus);
}

void Tester::callVisitChannels(int line, int numChannels)
{
    Tester_ChannelVisitor visitor;
    d_object->visitChannels(visitor);
    BMQTST_ASSERT_EQ_D(line, numChannels, visitor.d_numChannels);
}

void Tester::cancelHandle(int line, const bslstl::StringRef& handleName)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    HandleInfo&                    info = d_handleMap[handleName];
    if (!info.d_handle) {
        BMQTST_ASSERT_D(line, false);
        return;  // RETURN
    }

    info.d_handle->cancel();
}

void Tester::closeChannel(int line, const bslstl::StringRef& channelName)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    ChannelInfo&                   info = d_channelMap[channelName];

    if (!info.d_channel) {
        BMQTST_ASSERT_D(line, false);
        return;  // RETURN
    }

    info.d_channel->close();
}

void Tester::blockChannelIo(int line, const bslstl::StringRef& channelName)
{
    ChannelInfo* info = NULL;
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
        info = &d_channelMap[channelName];

        if (!info->d_channel) {
            BMQTST_ASSERT_D(line, false);
            return;  // RETURN
        }
    }
    info->d_blockMutex.lock();
}

void Tester::unblockChannelIo(int line, const bslstl::StringRef& channelName)
{
    ChannelInfo* info = NULL;
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
        info = &d_channelMap[channelName];

        if (!info->d_channel) {
            BMQTST_ASSERT_D(line, false);
            return;  // RETURN
        }
    }
    info->d_blockMutex.unlock();
}

void Tester::writeChannel(int                      line,
                          const bslstl::StringRef& channelName,
                          const bslstl::StringRef& data,
                          bsls::Types::Int64       highWatermark,
                          StatusCategory::Enum     statusCategory)
{
    bdlbb::Blob writeData(&d_blobBufferFactory,
                          bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&writeData, data.data(), data.length());

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    ChannelInfo&                   info = d_channelMap[channelName];

    if (!info.d_channel) {
        BMQTST_ASSERT_D(line, false);
        return;  // RETURN
    }

    Status writeStatus(bmqtst::TestHelperUtil::allocator());
    info.d_channel->write(&writeStatus, writeData, highWatermark);
    BMQTST_ASSERT_EQ_D(line, writeStatus.category(), statusCategory);
}

void Tester::readChannel(int                      line,
                         const bslstl::StringRef& channelName,
                         const bslstl::StringRef& data)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    ChannelInfo&                   info = d_channelMap[channelName];

    if (!info.d_channel) {
        BMQTST_ASSERT_D(line, false);
        return;  // RETURN
    }

    bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
    while (info.d_readData.length() < static_cast<int>(data.length()) &&
           (bsls::TimeUtil::getTimer() - startTime) <
               5 * bdlt::TimeUnitRatio::k_NS_PER_S) {
        bslmt::LockGuardUnlock<bslmt::Mutex> unlockGuard(&d_mutex);  // UNLOCK
        bslmt::ThreadUtil::microSleep(1000);
    }

    if (info.d_readData.length() < static_cast<int>(data.length())) {
        PRINT(bdlbb::BlobUtilHexDumper(&info.d_readData));
        BMQTST_ASSERT_D(line, false);
        return;  // RETURN
    }

    bsl::string readString(bmqtst::TestHelperUtil::allocator());
    readString.resize(data.length(), '7');
    bmqu::BlobUtil::readNBytes(&readString.front(),
                               info.d_readData,
                               bmqu::BlobPosition(),
                               data.length());

    bdlbb::BlobUtil::erase(&info.d_readData, 0, data.length());

    BMQTST_ASSERT_EQ_D(line, readString.c_str(), data);
}

void Tester::checkNoRead(int line, const bslstl::StringRef& channelName)
{
    bslmt::ThreadUtil::microSleep(5000);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    ChannelInfo&                   info = d_channelMap[channelName];
    BMQTST_ASSERT_EQ_D(line, info.d_readData.length(), 0);
}

void Tester::checkResultCallback(int                       line,
                                 const bslstl::StringRef&  handleName,
                                 ChannelFactoryEvent::Enum event,
                                 StatusCategory::Enum      statusCategory,
                                 const bslstl::StringRef&  channelName)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();

    HandleInfo& info = d_handleMap[handleName];

    while (info.d_resultCbCalls.empty() &&
           (bsls::TimeUtil::getTimer() - startTime) <
               5 * bdlt::TimeUnitRatio::k_NS_PER_S) {
        bslmt::LockGuardUnlock<bslmt::Mutex> unlockGuard(&d_mutex);
        bslmt::ThreadUtil::microSleep(1000);
    }

    if (info.d_resultCbCalls.empty()) {
        BMQTST_ASSERT_D(line, false);
        return;  // RETURN
    }

    ResultCbInfo& cbInfo = info.d_resultCbCalls.front();
    BMQTST_ASSERT_EQ_D(line, cbInfo.d_event, event);
    BMQTST_ASSERT_EQ_D(line, cbInfo.d_status.category(), statusCategory);
    if (cbInfo.d_status) {
        bsl::string channelNameStr(channelName);

        Channel::CloseFn closeFn(bsl::allocator_arg_t(),
                                 d_allocator_p,
                                 bdlf::BindUtil::bind(&Tester::channelCloseCb,
                                                      this,
                                                      channelNameStr));
        cbInfo.d_channel->onClose(closeFn);

        Channel::WatermarkFn watermarkFn(
            bsl::allocator_arg_t(),
            d_allocator_p,
            bdlf::BindUtil::bind(&Tester::channelWatermarkCb,
                                 this,
                                 channelNameStr,
                                 bdlf::PlaceHolders::_1));
        cbInfo.d_channel->onWatermark(watermarkFn);

        Channel::ReadCallback readCb(
            bsl::allocator_arg_t(),
            bmqtst::TestHelperUtil::allocator(),
            bdlf::BindUtil::bind(&Tester::channelReadCb,
                                 this,
                                 channelName,
                                 bdlf::PlaceHolders::_1,
                                 bdlf::PlaceHolders::_2,
                                 bdlf::PlaceHolders::_3));
        Status readStatus(bmqtst::TestHelperUtil::allocator());
        cbInfo.d_channel->read(&readStatus, 1, readCb);
        if (readStatus) {
            // If the read succeeds, then the channel isn't down yet
            d_channelMap[channelNameStr].d_channel = cbInfo.d_channel;
        }
    }
    else {
        // cout << "Failed result: " << cbInfo.d_status << endl;
    }

    info.d_resultCbCalls.pop_front();
}

void Tester::checkResultCallback(int                      line,
                                 const bslstl::StringRef& handleName,
                                 const bslstl::StringRef& channelName)
{
    checkResultCallback(line,
                        handleName,
                        CFE_CHANNEL_UP,
                        CAT_SUCCESS,
                        channelName);
}

void Tester::checkNoResultCallback(int                      line,
                                   const bslstl::StringRef& handleName)
{
    bslmt::ThreadUtil::microSleep(5000);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    HandleInfo&                    info = d_handleMap[handleName];
    BMQTST_ASSERT_D(line, info.d_resultCbCalls.empty());
}

void Tester::checkChannelClose(int                      line,
                               const bslstl::StringRef& channelName,
                               bool                     closed)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    ChannelInfo&                   info = d_channelMap[channelName];

    bsls::Types::Int64 waitTime = closed
                                      ? 5 * bdlt::TimeUnitRatio::k_NS_PER_S
                                      : 5 * bdlt::TimeUnitRatio::k_NS_PER_MS;

    bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
    while (info.d_channel.get() &&
           (bsls::TimeUtil::getTimer() - startTime) < waitTime) {
        bslmt::LockGuardUnlock<bslmt::Mutex> unlockGuard(&d_mutex);
        bslmt::ThreadUtil::microSleep(1000);
    }

    BMQTST_ASSERT_EQ_D(line, (!info.d_channel.get()), closed);
}

void Tester::checkChannelWatermark(int                        line,
                                   const bslstl::StringRef&   channelName,
                                   ChannelWatermarkType::Enum watermarkType)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
    ChannelInfo&                   info = d_channelMap[channelName];

    bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
    while (info.d_watermarkEvents.empty() &&
           (bsls::TimeUtil::getTimer() - startTime) <
               30 * bdlt::TimeUnitRatio::k_NS_PER_S) {
        bslmt::LockGuardUnlock<bslmt::Mutex> unlockGuard(&d_mutex);
        bslmt::ThreadUtil::microSleep(1000);
    }

    if (info.d_watermarkEvents.empty()) {
        BMQTST_ASSERT_D("line: " << line << ", no watermark events received",
                        false);
        return;  // RETURN
    }

    BMQTST_ASSERT_EQ_D(line, info.d_watermarkEvents.front(), watermarkType);

    info.d_watermarkEvents.pop_front();
}

void Tester::checkNoChannelWatermark(int                      line,
                                     const bslstl::StringRef& channelName)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
    ChannelInfo&                   info = d_channelMap[channelName];
    if (!info.d_watermarkEvents.empty()) {
        BMQTST_ASSERT_D(line, false);
        return;  // RETURN
    }
}

void Tester::checkChannelUri(int                      line,
                             const bslstl::StringRef& channelName,
                             const bslstl::StringRef& prefix)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
    ChannelInfo&                   info = d_channelMap[channelName];
    if (!info.d_channel) {
        BMQTST_ASSERT_D(line, false);
        return;  // RETURN
    }

    bslstl::StringRef uri = info.d_channel->peerUri();
    BMQTST_ASSERT_EQ_D(line,
                       uri.data(),
                       bdlb::StringRefUtil::strstr(uri, prefix).data());
}

void Tester::startFilteringLogs(const bsl::string& messageSubstring)
{
    d_ballObserver.createInplace(bmqtst::TestHelperUtil::allocator(),
                                 &bsl::cout,
                                 bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<ball::FilteringObserver> filtObserver;
    filtObserver.createInplace(bmqtst::TestHelperUtil::allocator(),
                               d_ballObserver,
                               bdlf::BindUtil::bind(&ballFilter,
                                                    messageSubstring,
                                                    bdlf::PlaceHolders::_1,
                                                    bdlf::PlaceHolders::_2));

    ball::LoggerManager::singleton().deregisterObserver(k_BALL_OBSERVER_NAME);
    ball::LoggerManager::singleton().registerObserver(filtObserver,
                                                      k_BALL_OBSERVER_NAME);
}

void Tester::checkFilteredLogs(int line, int expected)
{
    if (!d_ballObserver) {
        BMQTST_ASSERT_D(line, false);
        return;  // RETURN
    }

    BMQTST_ASSERT_EQ_D(line, d_ballObserver->numPublishedRecords(), expected);
}

void Tester::checkNumPreCreateCbCalls(int line, int expected)
{
    BMQTST_ASSERT_EQ_D(line,
                       static_cast<int>(d_preCreateCbCalls.size()),
                       expected);
}

NtcChannelFactory& Tester::object()
{
    return *d_object;
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test9_tlsClientFailsOnPlaintextServer()
// ------------------------------------------------------------------------
// PRE CREATION CB TEST
//
// Concerns:
//  a) An attempt to connect to a plaintext server with a TLS channel will fail
//  b) An attempt to connect to a TLS server with a plaintext channel will fail
//  c) A failed attempt to connect from a plaintext client doesn't cause a TLS
//  listener to close
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Upgrade Channel Cb Test");

    Tester t(bmqtst::TestHelperUtil::allocator());

    // Use TLS for both connections and listeners
    t.setEncryptionServer(true);

    // Concern 'a'
    t.init(L_);

    {
        ChannelUpgradeHandler upgradeCounter(
            bmqtst::TestHelperUtil::allocator());

        // Register a way to record upgrade events
        t.object().onUpgrade(upgradeCounter.makeOnUpgradeCb());

        // Shorten the timeout interval
        ntca::UpgradeOptions upgradeOptions;
        bsls::TimeInterval   deadline = bdlt::CurrentTime::now();
        // 5ms doesn't seem to interfere with the success test case, so this
        // should be sufficient enough
        deadline.addMilliseconds(5);
        upgradeOptions.setDeadline(deadline);
        t.object().setUpgradeOptions(upgradeOptions);

        BMQTST_ASSERT_EQ(0, t.object().start());
    }

    t.listen(L_, "listenHandle0", "127.0.0.1:5001", StatusCategory::e_SUCCESS);
    t.connect(L_,
              "connectHandle0",
              "listenHandle0",
              StatusCategory::e_SUCCESS);

    t.checkResultCallback(L_,
                          "listenHandle0",
                          ChannelFactoryEvent::e_CONNECT_FAILED,
                          StatusCategory::e_GENERIC_ERROR);
    t.checkNoResultCallback(L_, "channelHandle0");
}

static void test8_upgradeChannelTest()
// ------------------------------------------------------------------------
// PRE CREATION CB TEST
//
// Concerns:
//  a) 'upgradeChannelCb' is called for every channel upgraded
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Upgrade Channel Cb Test");

    Tester t(bmqtst::TestHelperUtil::allocator());

    // Use TLS for both connections and listeners
    t.setEncryptionServer(true);
    t.setEncryptionClient(true);

    // Concern 'a'
    t.init(L_);

    ChannelUpgradeHandler upgradeCounter(bmqtst::TestHelperUtil::allocator());

    // Register a way to record upgrade events
    t.object().onUpgrade(upgradeCounter.makeOnUpgradeCb());

    // Create a TLS channel
    t.listen(L_, "listenHandle", "127.0.0.1:5001", StatusCategory::e_SUCCESS);
    t.connect(L_, "connectHandle", "listenHandle", StatusCategory::e_SUCCESS);
    t.connect(L_, "connectHandle2", "listenHandle", StatusCategory::e_SUCCESS);

    // Confirm the channel was successfully created
    t.checkResultCallback(L_, "listenHandle", "listenChannel");
    t.checkResultCallback(L_, "connectHandle", "connectChannel");
    t.checkResultCallback(L_, "connectHandle2", "connectChannel2");

    // We expect each connect/listen pair to have received a successful
    // upgrade event
    upgradeCounter.checkAll();
    BMQTST_ASSERT_EQ_D("Unexpected number of channel upgrades",
                       4UL,
                       upgradeCounter.upgradeCounts());

    // Confirm the upgrade didn't mess up the received message
    t.writeChannel(L_, "listenChannel", "abcdef");
    t.readChannel(L_, "connectChannel", "abcdef");
}

static void test7_checkMultithreadListen()
{
    bmqtst::TestHelper::printTestName("Check Multithread Listen Test");

    const size_t k_NUM_THREADS = 10;
    const size_t k_RETRY_COUNT = 1000;

    struct LocalFuncs {
        static void
        resultCb(BSLA_UNUSED ChannelFactoryEvent::Enum event,
                 BSLA_UNUSED const bmqio::Status& status,
                 BSLA_UNUSED const bsl::shared_ptr<Channel>& channel)
        {
            // NOTHING
        }

        static void listenerThreadFunc(bslmt::Latch*   doneLatch_p,
                                       ChannelFactory* cf_p,
                                       int             port)
        {
            // PRECONDITIONS
            BSLS_ASSERT_SAFE(doneLatch_p);
            BSLS_ASSERT_SAFE(cf_p);

            ListenOptions options(bmqtst::TestHelperUtil::allocator());
            options.setEndpoint(bsl::string("127.0.0.1:") +
                                bsl::to_string(port));

            for (size_t i = 0; i < k_RETRY_COUNT; ++i) {
                bmqio::Status                               status;
                bslma::ManagedPtr<ChannelFactory::OpHandle> opHandle_mp;

                cf_p->listen(&status, &opHandle_mp, options, resultCb);

                if (status) {
                    bslmt::ThreadUtil::microSleep(2);
                    opHandle_mp->cancel();
                }
                else {
                    bslmt::ThreadUtil::microSleep(1);
                }
                bslmt::ThreadUtil::yield();
            }
            doneLatch_p->arrive();
        }
    };

    Tester t(bmqtst::TestHelperUtil::allocator());
    t.init(L_);

    const int port = t.findFreeEphemeralPort();

    // Create listener threads
    bslmt::Latch                           doneLatch(k_NUM_THREADS);
    bsl::vector<bslmt::ThreadUtil::Handle> handles(
        bmqtst::TestHelperUtil::allocator());
    handles.resize(k_NUM_THREADS);

    for (size_t i = 0; i < k_NUM_THREADS; ++i) {
        bslmt::ThreadAttributes attributes(
            bmqtst::TestHelperUtil::allocator());
        attributes.setThreadName("Listener-" + bsl::to_string(i + 1));

        bsl::function<void(void)> task = bdlf::BindUtil::bindS(
            bmqtst::TestHelperUtil::allocator(),
            LocalFuncs::listenerThreadFunc,
            &doneLatch,
            &t.object(),
            port);

        const int rc = bslmt::ThreadUtil::createWithAllocator(
            &handles[i],
            attributes,
            task,
            bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ_D(L_, rc, 0);
    }

    // Threads are starting to listen at their own pace,
    // try to stop/start the channel factory at the same time
    for (size_t i = 0; i < k_RETRY_COUNT; i++) {
        t.object().stop();
        t.object().start();
    }

    // Wait for all threads to finish
    const int rc = doneLatch.timedWait(bsls::SystemTime::nowRealtimeClock() +
                                       bsls::TimeInterval(5.0));
    BMQTST_ASSERT_EQ_D(L_, rc, 0);

    for (size_t i = 0; i < k_NUM_THREADS; i++) {
        bslmt::ThreadUtil::join(handles[i]);
    }
}

static void test6_preCreationCbTest()
// ------------------------------------------------------------------------
// PRE CREATION CB TEST
//
// Concerns:
//  a) 'channelPreCreationCb' is called for every channel created by the
//     NtcChannelFactory
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Pre Creation Cb Test");

    Tester t(bmqtst::TestHelperUtil::allocator());

    // Concern 'a'
    t.setPreCreateCb(true);
    t.init(L_);

    // Create 4 channel
    t.listen(L_, "listenHandle", "127.0.0.1:0");
    t.connect(L_, "connectHandle", "listenHandle");

    t.checkResultCallback(L_, "listenHandle", "listenChannel1");
    t.checkResultCallback(L_, "connectHandle", "connectChannel1");

    t.checkNumPreCreateCbCalls(L_, 2);

    // Make a second channel
    t.connect(L_, "connect2Handle", "listenHandle");

    t.checkResultCallback(L_, "connect2Handle", "connectChannel2");
    t.checkResultCallback(L_, "listenHandle", "listenChannel2");

    t.checkNumPreCreateCbCalls(L_, 4);
}

static void test5_visitChannelsTest()
// ------------------------------------------------------------------------
// VISIT CHANNELS TEST
//
// Concerns:
//  a) 'visitChannels' invokes the visitor with all the currently active
//     channels.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Cancel Handle Test");

    Tester t(bmqtst::TestHelperUtil::allocator());

    // Concerns 'a'
    t.init(L_);

    // Create 4 channel
    t.listen(L_, "listenHandle", "127.0.0.1:0");
    t.connect(L_, "connectHandle", "listenHandle");

    t.checkResultCallback(L_, "listenHandle", "listenChannel1");
    t.checkResultCallback(L_, "connectHandle", "connectChannel1");

    // Make a second channel
    t.connect(L_, "connect2Handle", "listenHandle");
    t.checkResultCallback(L_, "connect2Handle", "connectChannel2");
    t.checkResultCallback(L_, "listenHandle", "listenChannel2");

    t.callVisitChannels(L_, 4);

    t.closeChannel(L_, "listenChannel1");
    t.checkChannelClose(L_, "listenChannel1");
    t.checkChannelClose(L_, "connectChannel1");

    t.callVisitChannels(L_, 2);
}

static void test4_cancelHandleTest()
// ------------------------------------------------------------------------
// CANCEL HANDLE TEST
//
// Concerns:
//   a) Canceling a 'connect' handle before the NtcChannelFactory's
//      ResultCb is invoked prevents the ResultCb from being called, and
//      immediately closes the other end.
//   b) Canceling a 'listen' handle before a connect comes in prevents the
//      connection from being established
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Cancel Handle Test");

    // This test's original design was based on the assumption that `connect`
    // and `listen` operations are slower than `cancel`, however, this is
    // not the case in general.  We don't have non-invasive means to introduce
    // more synchronizations to NtcChannel now just to test this in a
    // controlled way, so this test is removed.
}

static void test3_watermarkTest()
// ------------------------------------------------------------------------
// WATERMARK TEST
//
// Concerns:
//   a) Hitting a channel's high watermark generates a high watermark and
//      low watermark event.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Watermark Test");

    Tester t(bmqtst::TestHelperUtil::allocator());

    // Concern 'a'
    t.init(L_);

    t.listen(L_, "listenHandle", "127.0.0.1:0");
    t.connect(L_, "connectHandle", "listenHandle");

    t.checkResultCallback(L_, "listenHandle", "listenChannel");
    t.checkResultCallback(L_, "connectHandle", "connectChannel");

    bsl::string largeMsg(10 * 1024 * 1024,
                         'a',
                         bmqtst::TestHelperUtil::allocator());

    // Block the IO thread to make sure our first write doesn't finish
    // before we get to the second one
    t.blockChannelIo(L_, "listenChannel");
    t.writeChannel(L_, "connectChannel", largeMsg);

    t.writeChannel(L_, "listenChannel", largeMsg, 10);
    t.writeChannel(L_, "listenChannel", largeMsg, 1, CAT_LIMIT);
    t.unblockChannelIo(L_, "listenChannel");

    t.checkChannelWatermark(L_, "listenChannel", WAT_HIGH);
    t.checkChannelWatermark(L_, "listenChannel", WAT_LOW);
}

static void test2_connectListenFailTest()
// ------------------------------------------------------------------------
// CONNECT LISTEN FAIL TEST
//
// Concerns:
//   a) Listening with a bad 'endpoint' fails as expected
//   b) Connecting with a bad 'endpoint' fails as expected
//   c) Connecting with 'autoReconnect' fails as expected
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Connect Listen Fail Test");

    Tester t(bmqtst::TestHelperUtil::allocator());
    t.init(L_);

    // Concern 'a'

    t.listen(L_, "h", "127.0.0.1", CAT_GENERIC);
    t.listen(L_, "h", "127.0.0.1:a", CAT_GENERIC);
    t.listen(L_, "h", "badhost:0", CAT_GENERIC);
    t.listen(L_, "h", "badport", CAT_GENERIC);

    // Concern 'b'

    // This fails asynchronously in NTC, rather than synchronously as in BTE:
    // t.connect(L_, "h", "localfoohost", CAT_GENERIC);

    t.connect(L_, "c1", "localfoohost");
    t.checkResultCallback(L_,
                          "c1",
                          ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED,
                          StatusCategory::e_CONNECTION);

    // This fails asynchronously in NTC, rather than synchronously as in BTE:
    // t.connect(L_, "h", "localhost:a", CAT_GENERIC);

    t.connect(L_, "c2", "localhost:a");
    t.checkResultCallback(L_,
                          "c2",
                          ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED,
                          StatusCategory::e_CONNECTION);

    // Concern 'c'

    bmqio::ConnectOptions options;
    options.setAutoReconnect(true);
    t.connect(L_, "h", "127.0.0.1:0", options, CAT_GENERIC);
}

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   a) Listening for a connection and connecting to the same port
//      establishes both connections
//   b) Closing a Channel closes both ends
//   c) Data written on one end can be read on the other end
//   d) Resolving a hostname to listen on works.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Breathing Test");

    Tester t(bmqtst::TestHelperUtil::allocator());
    t.init(L_);

    // Listen and connect work
    t.listen(L_, "listenHandle", "127.0.0.1:5001");
    t.connect(L_, "connectHandle", "listenHandle");

    t.checkResultCallback(L_, "listenHandle", "listenChannel");
    t.checkResultCallback(L_, "connectHandle", "connectChannel");

    // Make a second channel
    t.connect(L_, "connect2Handle", "listenHandle");
    t.checkResultCallback(L_, "connect2Handle", "connect2Channel");
    t.checkResultCallback(L_, "listenHandle", "listen2Channel");

    // Closing a channel closes both ends
    t.closeChannel(L_, "listen2Channel");
    t.checkChannelClose(L_, "listen2Channel");
    t.checkChannelClose(L_, "connect2Channel");

    t.writeChannel(L_, "listenChannel", "abcdef");
    t.readChannel(L_, "connectChannel", "abcdef");

    // Listen host resolution works
    t.init(L_);

    t.listen(L_, "listenHandle", "localhost:0");
    t.connect(L_, "connectHandle", "listenHandle");

    t.checkResultCallback(L_, "listenHandle", "listenChannel");
    t.checkResultCallback(L_, "connectHandle", "connectChannel");
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    bmqtst::TestHelperUtil::verbosityLevel() = 1;
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    ntcf::SystemGuard systemGuard(ntscfg::Signal::e_PIPE);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    case 2: test2_connectListenFailTest(); break;
    case 3: test3_watermarkTest(); break;
    case 4: test4_cancelHandleTest(); break;
    case 5: test5_visitChannelsTest(); break;
    case 6: test6_preCreationCbTest(); break;
    case 7: test7_checkMultithreadListen(); break;
    case 8: test8_upgradeChannelTest(); break;
    case 9: test9_tlsClientFailsOnPlaintextServer(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(0);
}
