// Copyright 2025 Bloomberg Finance L.P.
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

// mqbnet_connectiontype.h                        -*-C++-*-
#ifndef INCLUDED_MQBNET_CONNECTIONTYPE
#define INCLUDED_MQBNET_CONNECTIONTYPE

namespace BloombergLP {
namespace mqbnet {

// =====================
// struct ConnectionType
// =====================

struct ConnectionType {
    // Enum representing the type of session being negotiated, from that
    // side of the connection's point of view.
    enum Enum {
        e_UNKNOWN,
        e_CLUSTER_PROXY,   // Proxy (me) -> broker (outgoing)
        e_CLUSTER_MEMBER,  // Cluster node -> cluster node (both)
        e_CLIENT,          // Client or proxy -> me (incoming)
        e_ADMIN            // Admin client -> me (incoming)
    };
};

}  // close package namespace
}  // close enterprise namespace

#endif
