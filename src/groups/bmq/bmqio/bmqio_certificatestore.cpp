// Copyright 2024 Bloomberg Finance L.P.
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

// bmqio_certificatestore.cpp

#include <bsls_ident.h>
BSLS_IDENT_RCSID(bmqio_certificatestore_h, "$Id$ $CSID$")

#include <bmqio_certificatestore.h>

// ntc
#include <ntci_encryptioncertificate.h>
#include <ntci_interface.h>
#include <ntsa_error.h>

// bal
#include <ball_log.h>

// bsl
#include <bsl_fstream.h>
#include <bslma_allocator.h>

namespace {

const char LOG_CATEGORY[] = "MQBNET.CERTIFICATESTORE";

}  // close unnamed namespace

BALL_LOG_SET_NAMESPACE_CATEGORY(LOG_CATEGORY)

namespace BloombergLP {
namespace bmqio {

CertificateLoader::~CertificateLoader()
{
}

NtcCertificateLoader::NtcCertificateLoader(
    const bsl::shared_ptr<ntci::Interface>& interface)
: d_interface_sp(interface)
{
    BSLS_ASSERT(interface);
}

NtcCertificateLoader::NtcCertificateLoader(
    bslmf::MovableRef<NtcCertificateLoader> other)
: d_interface_sp(bslmf::MovableRefUtil::move(
      bslmf::MovableRefUtil::access(other).d_interface_sp))
{
}

NtcCertificateLoader&
NtcCertificateLoader::operator=(bslmf::MovableRef<NtcCertificateLoader> other)
{
    if (&other == this) {
        return *this;
    }

    NtcCertificateLoader& otherRef = other;
    d_interface_sp = bslmf::MovableRefUtil::move(otherRef.d_interface_sp);

    return *this;
}

bmqvt::ValueOrError<CertificateLoader::Certificate, Error>
NtcCertificateLoader::loadCertificateFromPath(
    const bsl::string& certificatePath)
{
    bsl::shared_ptr<ntci::EncryptionCertificate> certificate;

    // TODO Consider forwarding an allocator through this interface
    ntsa::Error err = d_interface_sp->loadCertificate(&certificate,
                                                      certificatePath);

    bmqvt::ValueOrError<Certificate, Error> result;
    if (!err) {
        result.makeValue(bslmf::MovableRefUtil::move(certificate));
    }
    else {
        result.makeError(bslmf::MovableRefUtil::move(err));
    }

    BSLS_ASSERT(!result.isUndefined());
    return result;
}

bmqvt::ValueOrError<CertificateLoader::Key, Error>
NtcCertificateLoader::loadKeyFromPath(const bsl::string& keyPath)
{
    bsl::shared_ptr<ntci::EncryptionKey> key;

    ntsa::Error err = d_interface_sp->loadKey(&key, keyPath);

    bmqvt::ValueOrError<Key, Error> result;
    if (!err) {
        result.makeValue(bslmf::MovableRefUtil::move(key));
    }
    else {
        result.makeError(err);
    }

    return result;
}

CertificateStore::CertificateStore(bslmf::MovableRef<CertificateStore> other)
: d_certificate_sp(bslmf::MovableRefUtil::move(
      bslmf::MovableRefUtil::access(other).d_certificate_sp))
, d_certificateAuthority_sp(bslmf::MovableRefUtil::move(
      bslmf::MovableRefUtil::access(other).d_certificateAuthority_sp))
, d_serverPrivateKey_sp(bslmf::MovableRefUtil::move(
      bslmf::MovableRefUtil::access(other).d_serverPrivateKey_sp))
{
}

CertificateStore&
CertificateStore::operator=(bslmf::MovableRef<CertificateStore> other)
{
    // TODO: The BDE docs show doing this but I'm not convinced it's correct
    CertificateStore& store(other);
    if (&store == this) {
        return *this;
    }

    d_certificate_sp          = bslmf::MovableRefUtil::move(d_certificate_sp);
    d_certificateAuthority_sp = bslmf::MovableRefUtil::move(
        d_certificateAuthority_sp);
    d_serverPrivateKey_sp = bslmf::MovableRefUtil::move(d_serverPrivateKey_sp);

    return *this;
}

// #review: do we want to support partially initialized CertificateStore?
// for clients where only CA is used for example
int CertificateStore::loadCertificateAuthority(CertificateLoader& loader,
                                               const bsl::string& caPath)
{
    bmqvt::ValueOrError<CertificateLoader::Certificate, Error>
        expectedCertificate = loader.loadCertificateFromPath(caPath);
    if (expectedCertificate.isError()) {
        const Error& error = expectedCertificate.error();
        BALL_LOG_ERROR << "While loading certificate authority rc: " << error
                       << ", path: " << caPath;
        return error.code();
    }

    return 0;
}

int CertificateStore::loadCertificate(CertificateLoader& loader,
                                      const bsl::string& certPath)
{
    bmqvt::ValueOrError<CertificateLoader::Certificate, Error>
        expectedCertificate = loader.loadCertificateFromPath(certPath);
    if (expectedCertificate.isError()) {
        const Error& error = expectedCertificate.error();
        BALL_LOG_ERROR << "While loading certificate rc: " << error
                       << ", path: " << certPath;
        return error.code();
    }

    return 0;
}

int CertificateStore::loadKey(CertificateLoader& loader,
                              const bsl::string& keyPath)
{
    bmqvt::ValueOrError<CertificateLoader::Certificate, Error>
        expectedCertificate = loader.loadCertificateFromPath(keyPath);
    if (expectedCertificate.isError()) {
        const Error& error = expectedCertificate.error();
        BALL_LOG_ERROR << "While loading key rc: " << error
                       << ", path: " << keyPath;
        return error.code();
    }

    return 0;
}

}  // close namespace bmqio
}  // close namespace BloombergLP
