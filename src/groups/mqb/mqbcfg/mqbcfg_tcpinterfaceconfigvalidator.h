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
// mqbcfg_tcpinterfaceconfigvalidator.h
#ifndef INCLUDED_MQBCFG_TCPINTERFACECONFIGVALIDATOR
#define INCLUDED_MQBCFG_TCPINTERFACECONFIGVALIDATOR

#include <bsls_ident.h>
BSLS_IDENT_RCSID(mqbcfg_tcpinterfaceconfigvalidator_h, "$Id$ $CSID$")
BSLS_IDENT_PRAGMA_ONCE

#include <mqbcfg_messages.h>

#include <ball_log.h>

/// PURPOSE: This component provdies a class to validate TcpInterfaceConfig
/// settings in the broker config.
///
/// CLASSES:
///  mqbcfg::TcpInterfaceConfigValidator: A predicate object that validates the
///  TCP interfaces in the broker config
///
/// DESCRIPTION:

namespace BloombergLP {
namespace mqbcfg {

/// @brief This class is a functor that validates the
/// `appConfig/networkInterfaces/tcpInterface/listeners` property of the broker
/// config.
class TcpInterfaceConfigValidator {
  private:
    // PRIVATE STATIC FUNCTIONS
    static int port(const mqbcfg::TcpInterfaceListener& listener);

    static bsl::string_view name(const mqbcfg::TcpInterfaceListener& listener);

    static bool isValidPort(const mqbcfg::TcpInterfaceListener& listener);

  public:
    // TYPES
    /// Codes to indicate the reason for a validation failure.
    enum ErrorCode {
        /// Indicates a config is valid. Guaranteed to equal zero.
        k_OK = 0,
        /// Indicates there were multiple interfaces with the same name.
        k_DUPLICATE_NAME = -1,
        /// Indicates there were multiple interfaces using the same ports.
        k_DUPLICATE_PORT = -2,
        /// Indicates a port number was passed outside of the valid port range.
        k_PORT_RANGE = -3
    };

    // ACCESSORS

    /// @brief Validate the TCP interface configuration.
    ///
    /// The TCP interface configuration is invalid unless:
    /// 1. The names of each network interface is unique
    /// 2. The ports of each network interface is unqiue
    /// 3. Ports passed are possible port values
    ///
    /// @returns An error code indicating success (`k_OK`) or a non-zero code
    /// indicating the cause of failure.
    ErrorCode operator()(const mqbcfg::TcpInterfaceConfig& config) const;
};

// ============================================================================
//                          INLINE FUNCTION DEFINITIONS
// ============================================================================

}  // close namespace mqbcfg
}  // close namespace BloombergLP
#endif