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

// bmqio_certificatestore.h
#ifndef INCLUDED_BMQIO_CERTIFICATESTORE
#define INCLUDED_BMQIO_CERTIFICATESTORE

#include <bsls_ident.h>
BSLS_IDENT_RCSID(bmqio_certificatestore_h, "$Id$ $CSID$")
BSLS_IDENT_PRAGMA_ONCE

/// PURPOSE: Provide a way to access broker certificates and keys
///
/// CLASSES:
///  bmqio::CertificateStore: A store manager for certificates and keys
///
/// DESCRIPTION: This component provides a store object,
/// `bmqio::CertificateStore`,
///   to provide an easy way to load certificates and keys from the filesystem
///   into a single interface.

// bmq
#include <bmqvt_valueorerror.h>

#include <ntsa_error.h>

// bsl
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslmf_movableref.h>

namespace BloombergLP {

namespace ntci {
class EncryptionCertificate;
class EncryptionKey;
class Interface;
}

namespace ntsa {
class Error;
}

namespace bmqio {

class Error : public ntsa::Error {
  public:
    Error()
    : ntsa::Error()
    {
    }

    Error(const ntsa::Error& error)
    : ntsa::Error(error)
    {
    }
};

class CertificateLoader {
  public:
    typedef bsl::shared_ptr<const ntci::EncryptionCertificate> Certificate;
    typedef bsl::shared_ptr<const ntci::EncryptionKey>         Key;

    virtual ~CertificateLoader();

    virtual bmqvt::ValueOrError<Certificate, Error>
    loadCertificateFromPath(const bsl::string& path) = 0;

    virtual bmqvt::ValueOrError<Key, Error>
    loadKeyFromPath(const bsl::string& path) = 0;
};

class NtcCertificateLoader : public CertificateLoader {
    bsl::shared_ptr<ntci::Interface> d_interface_sp;

  public:
    /// @brief Construct a certificate loader from an ntci::Interface
    ///
    /// @pre This operation is undefined unless interface is not null.
    NtcCertificateLoader(const bsl::shared_ptr<ntci::Interface>& interface);

    NtcCertificateLoader(bslmf::MovableRef<NtcCertificateLoader> other);
    NtcCertificateLoader&
    operator=(bslmf::MovableRef<NtcCertificateLoader> other);

    bmqvt::ValueOrError<Certificate, Error>
    loadCertificateFromPath(const bsl::string& path) BSLS_KEYWORD_OVERRIDE;

    bmqvt::ValueOrError<Key, Error>
    loadKeyFromPath(const bsl::string& path) BSLS_KEYWORD_OVERRIDE;

  private:
    // PRIVATE CONSTRUCTORS
    NtcCertificateLoader(const NtcCertificateLoader& other);
    NtcCertificateLoader& operator=(const NtcCertificateLoader& other);
};

/// Stores keys and certs from the TLS config
class CertificateStore {
  public:
    typedef bsl::shared_ptr<const ntci::EncryptionCertificate> Certificate;
    typedef bsl::shared_ptr<const ntci::EncryptionKey>         Key;

  private:
    Certificate d_certificate_sp;
    Certificate d_certificateAuthority_sp;
    Key         d_serverPrivateKey_sp;

  public:
    CertificateStore();

    CertificateStore(bslmf::MovableRef<CertificateStore> other);

    CertificateStore& operator=(bslmf::MovableRef<CertificateStore> other);

    /// @brief Load a PEM formatted certificate authority stored in the path
    /// specified by `caPath`.
    ///
    /// If caPath is a valid paths to a PEM formatted file, load it
    /// into memory and initialize certificateAuthority().
    ///
    /// @returns Non-zero if loading certificates failed.
    int loadCertificateAuthority(CertificateLoader& loader,
                                 const bsl::string& caPath);

    /// @brief Load a PEM formatted certificate authority stored in the path
    /// specified by `certPath`.
    ///
    /// If certPath is a valid paths to a PEM formatted file, load it
    /// into memory and initialize certificate().
    ///
    /// @returns Non-zero if loading certificates failed.
    int loadCertificate(CertificateLoader& loader,
                        const bsl::string& certPath);

    /// @brief Load a PEM formatted key stored in the path specified by
    /// `keyPath`.
    ///
    /// If keyPath is a valid paths to a PEM formatted file, load it
    /// into memory and initialize key().
    ///
    /// @returns Non-zero if loading keys failed.
    int loadKey(CertificateLoader& interface, const bsl::string& keyPath);

    /// @brief Get a handle to the loaded certificate.
    ///
    /// @returns A possibly null handle to the stored certificate.
    Certificate certificate() const;

    /// @brief Get a handle to the loaded certificate authority.
    ///
    /// @returns A possibly null handle to the stored certificate authority.
    Certificate certificateAuthority() const;

    /// @brief Get a handle to the loaded key.
    ///
    /// @returns A possibly null handle to the stored key.
    Key key() const;
};

// ============================================================================
//                          INLINE FUNCTION DEFINITIONS
// ============================================================================

inline CertificateStore::CertificateStore()
{
    // = default
}

inline CertificateStore::Certificate CertificateStore::certificate() const
{
    return d_certificate_sp;
}

inline CertificateStore::Certificate
CertificateStore::certificateAuthority() const
{
    return d_certificateAuthority_sp;
}

inline CertificateStore::Key CertificateStore::key() const
{
    return d_serverPrivateKey_sp;
}

}  // close namespace bmqio
}  // close namespace BloombergLP
#endif
