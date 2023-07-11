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

// mqbu_capacitymeter.h                                               -*-C++-*-
#ifndef INCLUDED_MQBU_CAPACITYMETER
#define INCLUDED_MQBU_CAPACITYMETER

//@PURPOSE: Provide a mechanism to meter capacity usage of a storage resource.
//
//@CLASSES:
//  mqbu::CapacityMeter: Mechanism to meter capacity usage of a storage
//
//@DESCRIPTION: 'mqbu::CapacityMeter' is a mechanism to control and meter the
// messages and bytes usage of a storage, automatically printing alarms when
// the storage is getting full, is full, and is back to a low usage.  It works
// with a concept of reserve and commit, where resources are reserved upfront
// and can then either be commited or released.
//
// The 'CapacityMeter' exposes five main operations:
//: o !reserve!:
//:      This method is used to request some resources, and check how much can
//:      be used.  It *must* be used before any other operation.  Once some
//:      amount of resource has been reserved, it can either be commited or
//:      released, but never more than the object authorized.
//:
//: o !release!:
//:      If some resources were reserved, but were not commited, they must be
//:      released to the object, so that they can be made available to other
//:      clients.
//:
//: o !commit!:
//:      Reserved resources, once used, must be commited to the object, to
//:      update the internal resource usage status and monitoring.
//:
//: o !commitUnreserved!:
//:      Perform 'reserve' and 'commit' in one shot, if the object has enough
//:      resources available.  This is an optimized optimistic operation useful
//:      when the user typically commits the data immediately after reserving
//:      it.
//:
//: o !remove!:
//:      A commited resource must be removed from the object once no longer
//:      used, in order to update the resource usage status and monitoring and
//:      make this resource available again to other clients.
//
/// Chainability
///------------
// An 'mqbu::CapacityMeter' can be given a parent meter upon creation.  In this
// case, this meter is assumed to be part of a container represented by the
// parent, and any operation will also be forwarded to the parent: when
// reserving resources, it will take into account not only the availability in
// the current meter, but also the availability in the parent one.
//
/// Enablement
///----------
// By default, an 'mqbu::CapacityMeter' is enabled, meaning that it effectively
// keeps track of the resources.  In certain situations, interfaces and APIs
// require to use a meter, but the object doesn't care about resource
// management: this is the case for example for a remote domain or queue.
// Because resource tracking has a cost (spinLock, ...), the meter can be
// entirely disabled.  Once disabled APIs such as 'reserve' will always
// authorize what was asked, but 'commit' or 'remove' will not update any
// internal state.  This means that a disabled meter is pretty much a void
// pass-through object.
//
/// Thread Safety
///-------------
// The 'mqbu::CapacityMeter' class is fully thread-safe (see
// 'bsldoc_glossary'), meaning that two threads can safely call any methods on
// the *same* *instance* without external synchronization.
//
/// Usage Example
///-------------
// This example shows typical usage of the 'CapacityMeter' object.
//
// First, let's create and configure a 'CapacityMeter' object:
//..
//  mqbu::CapacityMeter capacityMeter("myResource", allocator);
//  capacityMeter.setLimits(1000,         // 1 000 messages
//                          1024 * 1024); // 1 MB
//..
//
// The first thing to do then is reserve some resources, but we first need to
// compute how much resource we need:
//..
//  bsls::Types::Int64 requiredMessages;
//  bsls::Types::Int64 requiredBytes;
//  bsls::Types::Int64 reservedMessages;
//  bsls::Types::Int64 reservedBytes;
//
//  bmqp::PutMessageIterator putIterator;
//  while (putIterator.next() == 1) {
//    ++requiredMessages;
//    requiredBytes += putIt.messagePayloadSize();
//  }
//
//  // Let's now request the meter to reserve some resources for us:
//  capacityMeter.reserve(&reservedMessages,
//                        &reservedBytes,
//                        requiredMessages,
//                        requiredBytes);
//  // NOTE: availablesMessages will be <= requiredMessages, and similarly for
//  //       availableBytes and requiredBytes.
//..
//
// Now that we reserved some resources, we can commit messages, up to at most
// the returned 'availableMessages' and 'availableBytes'.
//..
//  bsls::Types::Int64 uncommittedMessages = reservedMessages;
//  bsls::Types::Int64 uncommittedBytes    = reservedBytes;
//  bsls::Types::Int64 availableMessages  = reservedMessages;
//  bsls::Types::Int64 availableBytes     = reservedBytes;
//
//  while (putIterator.next() == 1) {
//    if (   availableMessages == 0)
//        || availableBytes < putIt.messagePayloadSize()) {
//        // We have exhausted one of the two resources, we must NOT commit
//        // that message as this would violate our reservation.
//        break;
//    }
//    // Do something with the message, if needed
//    capacityMeter.commit(1, putIt.messagePayloadSize());
//
//    // Update counter of remaining capacity
//    --availableMessages;
//    availableBytes -= putIt.messagePayloadSize();
//    --uncommittedMessages;
//    uncommittedBytes -= putIt.messagePayloadSize();
//  }
//..
//
// Once we are done committing messages, we *MUST* release all the reserved
// resources allocated to us that we did not commit:
//..
//  capacityMeter.release(uncommittedMessages, uncommittedBytes);
//..

// MQB

#include <mqbcmd_messages.h>
#include <mqbu_resourceusagemonitor.h>

// BDE
#include <ball_log.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_spinlock.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbu {

// ===================
// class CapacityMeter
// ===================

/// Mechanism to meter capacity usage of a storage
class CapacityMeter {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBU.CAPACITYMETER");

  public:
    // TYPES
    enum CommitResult {
        // Enum representing the possible result value of the
        // 'commitUnreserved' operation.

        e_SUCCESS = 0  // operation was success
        ,
        e_LIMIT_MESSAGES = 1  // messages limit was hit
        ,
        e_LIMIT_BYTES = 2  // bytes limit was hit
    };

  private:
    // DATA
    bsl::string d_name;
    // Name of this object (typically the name of
    // the resource monitored by this meter);
    // internally used for log printing.

    bool d_isDisabled;
    // True if this meter is disabled (refer to
    // 'Enablement' section in the component level
    // documentation for more information).

    CapacityMeter* d_parent_p;
    // Optional parent meter

    ResourceUsageMonitor d_monitor;
    // Monitor for the bytes and messages capacity
    // of this meter

    bsls::Types::Int64 d_nbMessagesReserved;
    // Number of messages reserved

    bsls::Types::Int64 d_nbBytesReserved;
    // Number of bytes reserved

    mutable bsls::SpinLock d_lock;
    // SpinLock for synchronization of this
    // component

    // FRIENDS
    friend struct CapacityMeterUtil;

  private:
    // PRIVATE ACCESSORS

    /// Function invoked to print, if necessary, the specified
    /// `stateTransition` resulting from updating the value of the
    /// `d_monitor` after committing to it.
    void logOnMonitorStateTransition(
        ResourceUsageMonitorStateTransition::Enum stateTransition) const;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented
    CapacityMeter(const CapacityMeter&, bslma::Allocator*);
    CapacityMeter& operator=(const CapacityMeter&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(CapacityMeter, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new un-configured object having the specified `name` and
    /// using the specified `allocator`.
    CapacityMeter(const bsl::string& name, bslma::Allocator* allocator);

    /// Create a new un-configured object having the specified `name`, being
    /// a child of the specified `parent` meter and using the specified
    /// `allocator`.
    CapacityMeter(const bsl::string& name,
                  CapacityMeter*     parent,
                  bslma::Allocator*  allocator);

    // MANIPULATORS

    /// Configure this object to manage at most the specified `messages` and
    /// `bytes` and return a reference offering modifiable access to this
    /// object.  Note that if a parent is associated to this meter, this
    /// method will only change this object's configuration and not the
    /// parent's one.
    CapacityMeter& setLimits(bsls::Types::Int64 messages,
                             bsls::Types::Int64 bytes);

    /// Configure the high watermark thresholds for messages and bytes of
    /// this object to be the specified `messages` and `bytes` respectively,
    /// and return a reference offering modifiable access to this object.
    /// The threshold represents a percentage of the configured capacity and
    /// should be in the [0 - 1] range.  The low watermark thresholds will
    /// be half of the high thresholds.  Note that if a parent is associated
    /// to this meter, this method will only change this object's
    /// configuration and not the parent's one.
    CapacityMeter& setWatermarkThresholds(double messages, double bytes);

    /// Disable this meter and return a reference offering modifiable access
    /// to this object.  Note that the object must not have been used before
    /// being disabled, otherwise it may leave the parent (it any) in some
    /// undefined state where resources were commited to it, but will never
    /// be removed.
    CapacityMeter& disable();

    /// Request to reserve the specified `messages` and `bytes` resources.
    /// Populate the specified `nbMessagesAvailable` and `nbBytesAvailable`
    /// with the maximum respective messages and bytes which can be
    /// commited.  Any un-commited resource *MUST* be released with the
    /// `release()` method.  It a parent is associated to this meter,
    /// resources will also be reserved on it.  It is undefined behavior to
    /// commit more than the returned `nbMessagesAvailable` and
    /// `nbBytesAvailable`.
    void reserve(bsls::Types::Int64* nbMessagesAvailable,
                 bsls::Types::Int64* nbBytesAvailable,
                 bsls::Types::Int64  messages,
                 bsls::Types::Int64  bytes);

    /// Release the specified `messages` and `bytes` which were previously
    /// reserved and which have not been commited.  Note that only, and all,
    /// of the reserved resources which have not been commited must be
    /// released.
    void release(bsls::Types::Int64 messages, bsls::Types::Int64 bytes);

    /// Confirm usage of the specified `messages` and `bytes` which must
    /// have been previously reserved.  The behavior is undefined unless at
    /// least `messages` and `bytes` have been previously reserved.
    void commit(bsls::Types::Int64 messages, bsls::Types::Int64 bytes);

    /// This optimized optimistic operation will reserve and commit the
    /// specified `messages` and `bytes` resources on this object if it has
    /// enough capacity for it, or be a no-op otherwise.  Return whether the
    /// resources were successfully commited, or if it failed due to some
    /// limits being hit.
    CommitResult commitUnreserved(bsls::Types::Int64 messages,
                                  bsls::Types::Int64 bytes);

    /// Force commit usage of the specified `messages` and `bytes` which
    /// must *NOT* have been previously reserved; and do not perform any
    /// capacity checking.  This is mostly used for recovery startup where
    /// the capacity may have changed (and been reduced), yet we must ensure
    /// we load all messages and update the capacity.
    void forceCommit(bsls::Types::Int64 messages, bsls::Types::Int64 bytes);

    /// Confirm removal of the specified `messages` and `bytes` which must
    /// have been previously commited, and log any errors or warnings if the
    /// specified `silenceMode` flag is false.  Print a warning when either
    /// the messages or bytes monitors reaches the low watermark, unless the
    /// optionally specified `silentMode` is true.
    void remove(bsls::Types::Int64 messages,
                bsls::Types::Int64 bytes,
                bool               silentMode = false);

    /// Resets to zero the currently allocated messages and bytes resources;
    /// and removes them from the parent if any.  Note that this doesn't
    /// touch the currently reserved resources; so that this method can
    /// safely be called while having operations pending.
    void clear();

    // ACCESSORS

    /// Return the name of this object.
    const bsl::string& name() const;

    /// Return the number of messages currently commited to this meter.
    /// Note that for efficiency, this result should be cached in a local
    /// variable by the caller when possible.
    bsls::Types::Int64 messages() const;

    /// Return the number of bytes currently commited to this meter.  Note
    /// that for efficiency, this result should be cached in a local
    /// variable by the caller when possible.
    bsls::Types::Int64 bytes() const;

    /// Return the message capacity of this meter.
    bsls::Types::Int64 messageCapacity() const;

    /// Return the byte capacity of this meter.
    bsls::Types::Int64 byteCapacity() const;

    /// Return the parent of this meter, if any, or a null pointer
    /// otherwise.
    const CapacityMeter* parent() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level, int spacesPerLevel) const;

    /// Write a short summary of this meter's current and maximum capacity
    /// to the specified output `stream`.
    bsl::ostream& printShortSummary(bsl::ostream& stream) const;
};

// ========================
// struct CapacityMeterUtil
// ========================

/// Utility struct for holding functions useful when dealing with
/// CapacityMeter objects.
struct CapacityMeterUtil {
    /// Load the state of the specified `capacityMeter` into the specified
    /// `state`.
    static void loadState(mqbcmd::CapacityMeter* state,
                          const CapacityMeter&   capacityMeter);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------
// class CapacityMeter
// -------------------

inline CapacityMeter& CapacityMeter::disable()
{
    d_isDisabled = true;
    return *this;
}

inline const bsl::string& CapacityMeter::name() const
{
    return d_name;
}

inline bsls::Types::Int64 CapacityMeter::messages() const
{
    if (d_isDisabled) {
        return 0;  // RETURN
    }

    bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK
    return d_monitor.messages();
}

inline bsls::Types::Int64 CapacityMeter::bytes() const
{
    if (d_isDisabled) {
        return 0;  // RETURN
    }

    bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK
    return d_monitor.bytes();
}

inline bsls::Types::Int64 CapacityMeter::messageCapacity() const
{
    bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK
    return d_monitor.messageCapacity();
}

inline bsls::Types::Int64 CapacityMeter::byteCapacity() const
{
    bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK
    return d_monitor.byteCapacity();
}

inline const CapacityMeter* CapacityMeter::parent() const
{
    return d_parent_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
