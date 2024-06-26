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

// mqbc_partitionfsmobserver.h                                        -*-C++-*-
#ifndef INCLUDED_MQBC_PARTITIONFSMOBSERVER
#define INCLUDED_MQBC_PARTITIONFSMOBSERVER

//@PURPOSE: Provide an Interface for PartitionFSM Observer.
//
//@CLASSES:
//  mqbc::PartitionFSMObserver: Interface for a PartitionFSM observer
//
//@DESCRIPTION: 'mqbc::PartitionFSMObserver' is an interface used to notify
// observer events for PartitionFSM.

// MQB

#include <mqbc_partitionstatetable.h>

namespace BloombergLP {

namespace mqbc {

// ==========================
// class PartitionFSMObserver
// ==========================

/// This interface exposes notifications of events happening on in
/// PartitionFSM.
///
/// NOTE: This is purposely not a pure interface, each method has a default
///       void implementation, so that clients only need to implement the
///       ones they care about.
class PartitionFSMObserver {
  public:
    // CREATORS

    /// Destructor
    virtual ~PartitionFSMObserver();

    virtual void
    onTransitionToPrimaryHealed(int                            partitionId,
                                PartitionStateTableState::Enum oldState);
    virtual void
    onTransitionToReplicaHealed(int                            partitionId,
                                PartitionStateTableState::Enum oldState);

    /// Called by PartitionFSM when corresponding state transition from the
    /// specified `oldState` happens for the specified `paritionId`.
    virtual void
    onTransitionToUnknown(int                            partitionId,
                          PartitionStateTableState::Enum oldState);
};

}  // close package namespace
}  // close enterprise namespace

#endif
