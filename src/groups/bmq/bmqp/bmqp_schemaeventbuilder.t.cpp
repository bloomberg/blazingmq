// Copyright 2015-2023 Bloomberg Finance L.P.
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

// bmqp_schemaeventbuilder.t.cpp                                      -*-C++-*-
#include <bmqp_schemaeventbuilder.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_protocolutil.h>
#include <bmqscm_version.h>
#include <bmqt_queueflags.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_guid.h>
#include <bdlb_guidutil.h>
#include <bdlb_scopeexit.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsl_algorithm.h>
#include <bsl_fstream.h>
#include <bsl_ios.h>
#include <bsl_string.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    int                            rc;
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));

    struct Test {
        int                      d_line;
        bmqp::EncodingType::Enum d_encodingType;
    } k_DATA[] = {{
                      L_,
                      bmqp::EncodingType::e_BER,
                  },
                  {L_, bmqp::EncodingType::e_JSON}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];
        PVV(test.d_line << ": Testing " << test.d_encodingType << "encoding");

        bmqp::SchemaEventBuilder obj(&blobSpPool,
                                     test.d_encodingType,
                                     bmqtst::TestHelperUtil::allocator());

        PVV(test.d_line << ": Verifying accessors");
        ASSERT_EQ(obj.blob()->length(), 0);

        PVV(test.d_line << ": Create a message");
        bmqp_ctrlmsg::ControlMessage message(
            bmqtst::TestHelperUtil::allocator());
        bmqp_ctrlmsg::Status&        status = message.choice().makeStatus();
        status.code()                       = 123;
        status.message()                    = "Test";

        // Encode the message
        rc = obj.setMessage(message, bmqp::EventType::e_CONTROL);
        ASSERT_EQ(rc, 0);
        ASSERT_NE(obj.blob()->length(), 0);
        ASSERT_EQ(obj.blob()->length() % 4, 0);

        PVV(test.d_line << ": Decode and compare message");
        bmqp::Event event(obj.blob().get(),
                          bmqtst::TestHelperUtil::allocator());

        ASSERT_EQ(event.isValid(), true);
        ASSERT_EQ(event.isControlEvent(), true);

        bmqp_ctrlmsg::ControlMessage decoded(
            bmqtst::TestHelperUtil::allocator());
        rc = event.loadControlEvent(&decoded);
        ASSERT_EQ(rc, 0);

        ASSERT(decoded.choice().isStatusValue());
        ASSERT_EQ(decoded.choice().status().code(), status.code());
        ASSERT_EQ(decoded.choice().status().message(), status.message());

        PVV("Reset");
        obj.reset();
        ASSERT_EQ(obj.blob()->length(), 0);
    }
}

/// This is a helper function to test the routine of encoding the message,
/// writing to file, reading from file and decoding it, as described in
/// the plan in test2_decodeFromFile()'s documentation.
template <class TYPE>
void testDecodeFromFileHelper(bmqp::SchemaEventBuilder*       obj,
                              TYPE*                           decoded,
                              const TYPE&                     message,
                              const char*                     typeString,
                              bdlbb::PooledBlobBufferFactory* bufferFactory)
{
    BSLS_ASSERT(decoded != 0);
    BSLS_ASSERT(typeString != 0);

    int rc;

    // Encode the message
    rc = obj->setMessage(message, bmqp::EventType::e_CONTROL);
    ASSERT_EQ(rc, 0);
    ASSERT_NE(obj->blob()->length(), 0);

    bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
    bdlb::Guid         guid = bdlb::GuidUtil::generate();

    os << "msg_control_" << typeString << "_" << guid << ".bin" << bsl::ends;

    /// Functor invoked to delete the file at the specified `filePath`
    struct local {
        static void deleteFile(const char* filePath)
        {
            BSLS_ASSERT_OPT(bsl::remove(filePath) == 0);
        }
    };

    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bind(local::deleteFile, os.str().data()));

    // Dump blob into file
    bsl::ofstream ofile(os.str().data(), bsl::ios::binary);

    BSLS_ASSERT(ofile.good() == true);

    const int blobLen = obj->blob()->length();
    char*     buf     = new char[blobLen];
    bdlbb::BlobUtil::copy(buf, *obj->blob(), 0, blobLen);
    ofile.write(buf, blobLen);
    ofile.close();
    bsl::memset(buf, 0, blobLen);

    obj->reset();
    ASSERT_EQ(obj->blob()->length(), 0);

    // Read blob from file
    bsl::ifstream ifile(os.str().data(), bsl::ios::binary);

    BSLS_ASSERT(ifile.good() == true);

    ifile.read(buf, blobLen);
    ifile.close();

    bsl::shared_ptr<char> dataBufferSp(buf,
                                       bslstl::SharedPtrNilDeleter(),
                                       bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer     dataBlobBuffer(dataBufferSp, blobLen);
    bdlbb::Blob blb(bufferFactory, bmqtst::TestHelperUtil::allocator());

    blb.appendDataBuffer(dataBlobBuffer);
    blb.setLength(blobLen);

    PVVV("Decode event");
    bmqp::Event event(&blb, bmqtst::TestHelperUtil::allocator());

    ASSERT_EQ(event.isValid(), true);
    ASSERT_EQ(event.isControlEvent(), true);

    rc = event.loadControlEvent(decoded);
    ASSERT_EQ(rc, 0);

    delete[] buf;
}

static void test2_bestEncodingSupported()
// --------------------------------------------------------------------
// BEST ENCODING SUPPORTED
//
// Concerns:
//   Verify that bmqp::SchemaEventBuilderUtil can determine the best
//   encoding supported given a remote feature set.
//
// Testing:
//   bestEncodingSupported
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BEST ENCODING SUPPORTED");

    PV("Empty remote feature set");
    {
        // This is TEMPORARY only until all clients are properly advertising
        // their encoding features support; after that it should return INVALID
        ASSERT_EQ(bmqp::EncodingType::e_BER,
                  bmqp::SchemaEventBuilderUtil::bestEncodingSupported(""));
    }

    PV("Remote feature set only supports one encoding");
    {
        bsl::string features1(bmqtst::TestHelperUtil::allocator());
        features1.append(bmqp::EncodingFeature::k_FIELD_NAME)
            .append(":")
            .append(bmqp::EncodingFeature::k_ENCODING_BER);

        ASSERT_EQ(
            bmqp::EncodingType::e_BER,
            bmqp::SchemaEventBuilderUtil::bestEncodingSupported(features1));

        bsl::string features2(bmqtst::TestHelperUtil::allocator());
        features2.append(bmqp::EncodingFeature::k_FIELD_NAME)
            .append(":")
            .append(bmqp::EncodingFeature::k_ENCODING_JSON);

        ASSERT_EQ(
            bmqp::EncodingType::e_JSON,
            bmqp::SchemaEventBuilderUtil::bestEncodingSupported(features2));
    }

    PV("Remote feature set supports all encodings");
    {
        bsl::string features(bmqtst::TestHelperUtil::allocator());
        features.append(bmqp::EncodingFeature::k_FIELD_NAME)
            .append(":")
            .append(bmqp::EncodingFeature::k_ENCODING_BER)
            .append(",")
            .append(bmqp::EncodingFeature::k_ENCODING_JSON);

        ASSERT_EQ(
            bmqp::EncodingType::e_BER,
            bmqp::SchemaEventBuilderUtil::bestEncodingSupported(features));
    }
}

static void testN1_decodeFromFile()
// --------------------------------------------------------------------
// DECODE FROM FILE
//
// Concerns:
//   bmqp::SchemaEventBuilder encodes bmqp::Event so that binary data
//   can be stored into a file and then restored and decoded back.
//
// Plan:
//   1. Using bmqp::SchemaEventBuilder encode bmqp::EventType::e_CONTROL
//      event with one bmqp_ctrlmsg::Status message.
//   2. Store binary representation of this event into a file.
//   3. Read this file, decode event and verify that it contains a message
//      with expected status code and data.
//
// Testing:
// template <class TYPE>
// int
// SchemaEventBuilder::setMessage(const TYPE&     message,
//                                EventType::Enum type)
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DECODE FROM FILE");

    const int                      k_SIZE = 512;
    bdlbb::PooledBlobBufferFactory bufferFactory(
        k_SIZE,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));

    struct Test {
        int                      d_line;
        bmqp::EncodingType::Enum d_encodingType;
    } k_DATA[] = {{
                      L_,
                      bmqp::EncodingType::e_BER,
                  },
                  {L_, bmqp::EncodingType::e_JSON}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];
        PVV(test.d_line << ": Testing " << test.d_encodingType << " encoding");

        bmqp::SchemaEventBuilder obj(&blobSpPool,
                                     test.d_encodingType,
                                     bmqtst::TestHelperUtil::allocator());

        PVV(test.d_line << ": Status message");
        {
            bmqp_ctrlmsg::ControlMessage message(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::ControlMessage decoded(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::Status& status = message.choice().makeStatus();
            status.code()                = 123;
            status.message()             = "Test";

            testDecodeFromFileHelper(&obj,
                                     &decoded,
                                     message,
                                     "status",
                                     &bufferFactory);

            ASSERT(decoded.choice().isStatusValue());
            ASSERT_EQ(decoded.choice().status().code(), status.code());
            ASSERT_EQ(decoded.choice().status().message(), status.message());
        }

        PVV(test.d_line << ": Negotiation message - ClientIdentity");
        {
            bmqp_ctrlmsg::NegotiationMessage message(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::NegotiationMessage decoded(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::ClientIdentity&    ci = message.makeClientIdentity();

            ci.protocolVersion() = bmqp::Protocol::k_VERSION;
            ci.sdkVersion()      = bmqscm::Version::versionAsInt();
            ci.sdkLanguage()     = bmqp_ctrlmsg::ClientLanguage::E_CPP;
            ci.clientType()      = bmqp_ctrlmsg::ClientType::E_TCPCLIENT;
            ci.pid()             = 9999;
            ci.sessionId()       = 123;
            ci.hostName()        = "host_name";
            ci.processName()     = "proc_name";

            // Ensure 'guidInfo' contains default values
            bmqp_ctrlmsg::GuidInfo& gg = ci.guidInfo();
            ASSERT_EQ("", gg.clientId());
            ASSERT_EQ(0, gg.nanoSecondsFromEpoch());

            testDecodeFromFileHelper(&obj,
                                     &decoded,
                                     message,
                                     "nego_client_client_identity",
                                     &bufferFactory);

            ASSERT_EQ(decoded.isClientIdentityValue(), true);
            ASSERT_EQ(decoded.isBrokerResponseValue(), false);
            ASSERT_EQ(decoded.isReverseConnectionRequestValue(), false);
            ASSERT_EQ(decoded.isUndefinedValue(), false);

            bmqp_ctrlmsg::ClientIdentity& dci = decoded.clientIdentity();

            ASSERT_EQ(ci.protocolVersion(), dci.protocolVersion());
            ASSERT_EQ(ci.sdkVersion(), dci.sdkVersion());
            ASSERT_EQ(ci.sdkLanguage(), dci.sdkLanguage());
            ASSERT_EQ(ci.clientType(), dci.clientType());
            ASSERT_EQ(ci.pid(), dci.pid());
            ASSERT_EQ(ci.sessionId(), dci.sessionId());
            ASSERT_EQ(ci.hostName(), dci.hostName());
            ASSERT_EQ(ci.processName(), dci.processName());

            bmqp_ctrlmsg::GuidInfo& dgg = dci.guidInfo();

            ASSERT_EQ(gg.clientId(), dgg.clientId());
            ASSERT_EQ(gg.nanoSecondsFromEpoch(), dgg.nanoSecondsFromEpoch());
        }

        PVV(test.d_line << ": Negotiation message - ClientIdentity (GUIDS)");
        {
            bmqp_ctrlmsg::NegotiationMessage message(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::NegotiationMessage decoded(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::ClientIdentity&    ci = message.makeClientIdentity();

            ci.protocolVersion() = bmqp::Protocol::k_VERSION;
            ci.sdkVersion()      = bmqscm::Version::versionAsInt();
            ci.sdkLanguage()     = bmqp_ctrlmsg::ClientLanguage::E_CPP;
            ci.clientType()      = bmqp_ctrlmsg::ClientType::E_TCPCLIENT;
            ci.pid()             = 9999;
            ci.sessionId()       = 123;
            ci.hostName()        = "host_name";
            ci.processName()     = "proc_name";

            bmqp_ctrlmsg::GuidInfo& gg = ci.guidInfo();
            gg.clientId()              = "0A0B0C0D0E0F";
            gg.nanoSecondsFromEpoch()  = 1324512000;

            testDecodeFromFileHelper(&obj,
                                     &decoded,
                                     message,
                                     "nego_client_client_identity_guids",
                                     &bufferFactory);

            ASSERT_EQ(decoded.isClientIdentityValue(), true);
            ASSERT_EQ(decoded.isBrokerResponseValue(), false);
            ASSERT_EQ(decoded.isReverseConnectionRequestValue(), false);
            ASSERT_EQ(decoded.isUndefinedValue(), false);

            bmqp_ctrlmsg::ClientIdentity& dci = decoded.clientIdentity();

            ASSERT_EQ(ci.protocolVersion(), dci.protocolVersion());
            ASSERT_EQ(ci.sdkVersion(), dci.sdkVersion());
            ASSERT_EQ(ci.sdkLanguage(), dci.sdkLanguage());
            ASSERT_EQ(ci.clientType(), dci.clientType());
            ASSERT_EQ(ci.pid(), dci.pid());
            ASSERT_EQ(ci.sessionId(), dci.sessionId());
            ASSERT_EQ(ci.hostName(), dci.hostName());
            ASSERT_EQ(ci.processName(), dci.processName());

            bmqp_ctrlmsg::GuidInfo& dgg = dci.guidInfo();

            ASSERT_EQ(gg.clientId(), dgg.clientId());
            ASSERT_EQ(gg.nanoSecondsFromEpoch(), dgg.nanoSecondsFromEpoch());
        }

        PVV(test.d_line << ": Negotiation message - BrokerResponse");
        {
            const int                        k_BROKER_VERSION = 3;
            bmqp_ctrlmsg::NegotiationMessage message(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::NegotiationMessage decoded(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::BrokerResponse&    br = message.makeBrokerResponse();
            br.protocolVersion()                = bmqp::Protocol::k_VERSION;
            br.brokerVersion()                  = k_BROKER_VERSION;
            br.isDeprecatedSdk()                = false;
            br.result().code()                  = 0;
            br.result().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
            br.brokerIdentity()    = bmqp_ctrlmsg::ClientIdentity();

            bmqp_ctrlmsg::ClientIdentity& ci = br.brokerIdentity();
            ci.protocolVersion()             = bmqp::Protocol::k_VERSION;
            ci.sdkVersion()                  = bmqscm::Version::versionAsInt();
            ci.sdkLanguage() = bmqp_ctrlmsg::ClientLanguage::E_CPP;
            ci.clientType()  = bmqp_ctrlmsg::ClientType::E_TCPCLIENT;
            ci.pid()         = 9999;
            ci.sessionId()   = 123;
            ci.hostName()    = "host_name";
            ci.processName() = "proc_name";
            ci.clusterName() = "cluster_name";

            testDecodeFromFileHelper(&obj,
                                     &decoded,
                                     message,
                                     "nego_client_broker_response",
                                     &bufferFactory);

            ASSERT_EQ(decoded.isClientIdentityValue(), false);
            ASSERT_EQ(decoded.isBrokerResponseValue(), true);
            ASSERT_EQ(decoded.isReverseConnectionRequestValue(), false);
            ASSERT_EQ(decoded.isUndefinedValue(), false);

            bmqp_ctrlmsg::BrokerResponse& dbr = message.makeBrokerResponse();
            ASSERT_EQ(dbr.protocolVersion(), br.protocolVersion());
            ASSERT_EQ(dbr.brokerVersion(), br.brokerVersion());
            ASSERT_EQ(dbr.isDeprecatedSdk(), br.isDeprecatedSdk());
            ASSERT_EQ(dbr.result().code(), br.result().code());
            ASSERT_EQ(dbr.result().category(), br.result().category());

            bmqp_ctrlmsg::ClientIdentity& dci = dbr.brokerIdentity();
            ASSERT_EQ(ci.protocolVersion(), dci.protocolVersion());
            ASSERT_EQ(ci.sdkVersion(), dci.sdkVersion());
            ASSERT_EQ(ci.sdkLanguage(), dci.sdkLanguage());
            ASSERT_EQ(ci.clientType(), dci.clientType());
            ASSERT_EQ(ci.pid(), dci.pid());
            ASSERT_EQ(ci.sessionId(), dci.sessionId());
            ASSERT_EQ(ci.hostName(), dci.hostName());
            ASSERT_EQ(ci.processName(), dci.processName());
            ASSERT_EQ(ci.clusterName(), dci.clusterName());
        }

        PVV(test.d_line << ": OpenQueue message");
        {
            bmqp_ctrlmsg::ControlMessage message(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::ControlMessage decoded(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::OpenQueue&     openQueue =
                message.choice().makeOpenQueue();

            bmqp_ctrlmsg::QueueHandleParameters params(
                bmqtst::TestHelperUtil::allocator());
            bsls::Types::Uint64                 flags = 0;

            bmqt::QueueFlagsUtil::setWriter(&flags);
            bmqt::QueueFlagsUtil::setAck(&flags);

            params.uri()        = "bmq://bmq.test.mem.priority/test-queue";
            params.flags()      = flags;
            params.qId()        = 0;
            params.readCount()  = 0;
            params.writeCount() = 1;
            params.adminCount() = 0;

            openQueue.handleParameters() = params;

            testDecodeFromFileHelper(&obj,
                                     &decoded,
                                     message,
                                     "open_queue",
                                     &bufferFactory);

            ASSERT(decoded.choice().isOpenQueueValue());

            bmqp_ctrlmsg::OpenQueue& decOpenQueue =
                decoded.choice().openQueue();

            ASSERT(decOpenQueue == openQueue);
        }

        PVV(test.d_line << ": OpenQueueResponse message");
        {
            bmqp_ctrlmsg::ControlMessage message(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::ControlMessage decoded(
                bmqtst::TestHelperUtil::allocator());
            bmqp_ctrlmsg::OpenQueueResponse& openQueueResponse =
                message.choice().makeOpenQueueResponse();
            bmqp_ctrlmsg::OpenQueue& openQueue =
                openQueueResponse.originalRequest();

            bmqp_ctrlmsg::QueueHandleParameters params(
                bmqtst::TestHelperUtil::allocator());
            bsls::Types::Uint64                 flags = 0;

            bmqt::QueueFlagsUtil::setWriter(&flags);
            bmqt::QueueFlagsUtil::setAck(&flags);

            params.uri()        = "bmq://bmq.test.mem.priority/test-queue";
            params.flags()      = flags;
            params.qId()        = 0;
            params.readCount()  = 0;
            params.writeCount() = 1;
            params.adminCount() = 0;

            openQueue.handleParameters() = params;
            bmqp_ctrlmsg::RoutingConfiguration config;
            config.flags() = bmqp_ctrlmsg::RoutingConfigurationFlags ::
                E_DELIVER_CONSUMER_PRIORITY;

            openQueueResponse.routingConfiguration() = config;

            testDecodeFromFileHelper(&obj,
                                     &decoded,
                                     message,
                                     "open_queue_response",
                                     &bufferFactory);

            bmqp_ctrlmsg::OpenQueueResponse& decOpenQueueResponse =
                decoded.choice().openQueueResponse();
            ASSERT(decOpenQueueResponse == openQueueResponse);
        }
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 2: test2_bestEncodingSupported(); break;
    case 1: test1_breathingTest(); break;
    case -1: testN1_decodeFromFile(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
