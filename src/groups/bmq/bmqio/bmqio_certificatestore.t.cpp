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

// bmqio_certificatestore.t.cpp                                       -*-C++-*-
#include <bmqio_certificatestore.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// gmock
#include <gmock/gmock.h>

// NTC
#include <ntca_interfaceconfig.h>
#include <ntcf_system.h>
#include <ntci_interface.h>
#include <ntsa_error.h>
#include <ntsa_ipaddress.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_allocatorutil.h>
#include <bslmf_assert.h>
#include <bslmf_movableref.h>
#include <bsls_assert.h>
#include <bsls_byteorder.h>
#include <bslstl_sharedptr.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bmqio;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

bsl::shared_ptr<ntci::Interface> makeInterface(
    bsl::allocator<unsigned char> allocator = bsl::allocator<unsigned char>())
{
    bsl::shared_ptr<bdlbb::BlobBufferFactory> blobBufferFactory =
        bsl::allocate_shared<bdlbb::PooledBlobBufferFactory>(allocator,
                                                             0xFFFF);

    return ntcf::System::createInterface(
        ntca::InterfaceConfig(),
        bslmf::MovableRefUtil::move(blobBufferFactory),
        bslma::AllocatorUtil::adapt(allocator));
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

class NtcCertificateLoaderTest : public bmqtst::Test {
  protected:
    bsl::shared_ptr<ntci::Interface> d_interface;

    NtcCertificateLoaderTest()
    : d_interface(makeInterface(bmqtst::TestHelperUtil::allocator()))
    {
    }
};

BMQTST_TEST_F(NtcCertificateLoaderTest, BreathingTest)
{
    NtcCertificateLoader certLoader(d_interface);
    CertificateLoader*   parent = &certLoader;
    (void)parent;
}

class CertificateStoreTest : protected bmqtst::Test {
  public:
    CertificateStoreTest() {}
};

BMQTST_TEST_F(CertificateStoreTest, BreathingTest)
{
    CertificateStore store;
    BMQTST_ASSERT(!store.certificate());
    BMQTST_ASSERT(!store.certificateAuthority());
    BMQTST_ASSERT(!store.key());
}

class MockCertificateLoader : public CertificateLoader {
  public:
    MOCK_METHOD((bmqvt::ValueOrError<CertificateLoader::Certificate, Error>),
                loadCertificateFromPath,
                (const bsl::string& path));

    MOCK_METHOD((bmqvt::ValueOrError<CertificateLoader::Key, Error>),
                loadKeyFromPath,
                (const bsl::string& path));
};

BMQTST_TEST_F(CertificateStoreTest, CertificateLoad)
{
    using namespace ::testing;

    typedef bmqvt::ValueOrError<CertificateStore::Certificate, bmqio::Error>
        Expected;

    Expected result(bmqtst::TestHelperUtil::allocator());
    result.makeError();
    MockCertificateLoader loader;
    EXPECT_CALL(loader, loadCertificateFromPath(_))
        .WillRepeatedly(Return(result));

    CertificateStore store;
    bsl::string      certPath("fake.crt", bmqtst::TestHelperUtil::allocator());
    int              rc = store.loadCertificate(loader, certPath);
    BMQTST_ASSERT_EQ(0, rc);
}

// ========================================================================
//                                  MAIN
// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqtst::runTest(_testCase);

    // gmock uses the default allocator, so we only check global allocator
    // usage
    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
