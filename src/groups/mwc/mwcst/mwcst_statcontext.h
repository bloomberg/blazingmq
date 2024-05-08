// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mwcst_statcontext.h                                                -*-C++-*-
#ifndef INCLUDED_MWCST_STATCONTEXT
#define INCLUDED_MWCST_STATCONTEXT

//@PURPOSE: Provide a context to manage a set of user-defined stat values.
//
//@CLASSES:
// mwcst::StatContextConfiguration : configuration for a StatContext
// mwcst::StatContextIterator      : iterator of subcontexts
// mwcst::StatContext              : the stat context
//
//@SEE_ALSO:
//  mwcst_statvalue
//  mwcst_statutil
//  mwcst_statcontexttableinfoprovider
//
//@DESCRIPTION: This component defines a mechanism, 'mwcst::StatContext', for
// managing a hierarchical set of user-defined statistics.  It is intended to
// be used as a generic container of an application's various statistics.
//
/// Statistics
///----------
// The statistics themselves are maintained by a set of 'mwcst::StatValue'
// objects owned by the 'StatContext'.  The 'StatContext' itself is mainly
// responsible for forwarding updates to the correct 'StatValue' and providing
// a simple interface to its values.  Think about a "value" as a variable,
// containing some metric that the application cares about, such as the number
// or bytes or messages received via an interface.  The application will update
// the value every time a message is received, and in return the application
// can obtain various statistics about the value, such as rates, or
// min/max/average over a given period of time.  See the documentation of
// 'mwcst::StatUtil' for the types of statistics that are currently supported
// for a value.
//
// The statistics to be maintained by a 'StatContext' are defined by a
// 'mwcst::StatContextConfiguration' object provided by the application when a
// 'StatContext' is created.  This configuration tells the 'StatContext' what
// values to maintain, as well as which statistics of those values the user is
// interested in.
//
// The values and statistics can have names and types associated with them.
// For example, if the application is interested in keeping track of the number
// of messages being received over a connection, it might define a statistic
// for a value, of type 'DMCST_NUM_INCREMENTS', and name this statistic "Number
// of Messages".  While these names aren't necessary for stat collection, they
// are useful to components that will want to process the 'StatContext'.  See
// 'mwcst::StatContextTableInfoProvider' for an example of a generic component
// that can be used with the 'mwcst::TableUtil' utility to print a table
// 'StatContext' to a stream.
//
/// Stat Tables
///-----------
// Statistics come in two flavors: singleton-type statistics such as process
// CPU and memory usage, and table-like statistics such as the set of all TCP
// clients or connections.  A 'StatContext' can be used for both of these
// cases.  For the second (table) case, just create a context or subcontext
// with 'isTable == true'.  This will allow you to use 'addSubTable' to add
// sub tables (which will themselves be tables) to the 'StatContext'. Then
// these sub tables can each be used to track an individual client or
// connection, for example.
//
// A parent 'StatContext' can be queried for its own statistics, or for the
// combined statistics of itself and all its children.  This allows for
// arbitrarily deep hierarchies that are automatically aggregated whenever a
// snapshot is taken.  For the singleton-like use case, either a table (with
// no children) or non-table 'StatContext' can be used equally well.
//
// In terms of usage, the only difference between a table and non-table
// 'StatContext' is that 'addSubTable' can only be called on a table and
// 'addSubContext' can only be called on a non-table.
//
/// Thread Safety
///-------------
// The 'StatContext' is designed to be as fast as possible for updating stats,
// and manages to make updates lock-free and subcontext and subtable additions
// almost lock-free.  A 'StatContext' is expected to be updated by possibly
// many threads, all contributing their statistics and creating new
// subcontexts, but it *must* be snapshotted and processed by only one thread
// at a time.
//
// In simpler terms, only 'addSubContext', 'addSubTable', 'adjustValue',
// 'reportValue', and 'setValue' are thread-safe.  All other functions should
// be considered not thread safe.
//
/// Intended Usage Pattern
///----------------------
// The easiest way to use a 'StatContext' to collect statistics for an
// application is to define a single top-level 'StatContext' for the
// application and to create sub contexts for the various sub-systems to be
// tracked.  Having a single top-level 'StatContext' makes it simple to
// update all the application stats at once.
//
// The application must do 3 things after the 'StatContext' is created.  First
// it needs to maintain the values when some event related to these values
// occur (like for example, the reception of a message via an interface).
//
// Second, it needs to request the 'StatContext' to process statistics on a
// regular basis, in order to maintain statistics like rates.  To process the
// statistics, 'snapshot' and 'cleanup' must be called.  'snapshot' will update
// all 'StatContexts' with current values, while 'cleanup' will delete any
// subcontexts that are no longer referenced by the application.  'cleanup'
// isn't automatic because an application might want to keep the subcontexts
// around until it has printed them somehow.  If 'snapshot' automatically
// deleted any unreferenced subcontexts, the application would have no way of
// knowing about any contexts that were created and destroyed between
// calls to 'snapshot'.
//
// Third and finally, the application needs to use these statistics, like for
// example log them on a regular basis, or publish them to some dashboard
// monitoring function. To do this the application accesses all the snapshotted
// values and presumably prints them in some way.  For example,
// 'mwcst::StatContextTableInfoProvider' can be used in a call to
// 'mwcst::TableUtil' to print a table 'StatContext' to a stream.
//
/// Serializable Updates
///--------------------
// A 'StatContext' can be configured to maintain a 'diff' of its state between
// the last two snapshots by providing it with a serializable
// 'mwcstm::StatContextUpdate'.  On each snapshot, the 'StatContext' will
// update the values of the 'mwcstm::StatContextUpdate' to represent the value
// changes in that snapshot.  This update may then be applied to another
// 'StatContext', which will add or remove subcontexts and update its values
// based on the update.  Since the update component is serializable, this
// context can reside in another process.
//
// 'StatContext' additionally provides a way to generate a complete
// 'mwcstm::StatContextUpdate' based on its current state.  This is useful to
// create an update containing the full state of the 'StatContext' since it was
// created, rather than just the incremental changes since the last snapshot.
//
/// Basic Usage Example
///-------------------
// In this example we will be keeping track of the volumes of data moving
// in and out of a particular network interface.  The data we want to keep
// track of is the current real time volumes.  Specifically we want to keep
// track of:
//: o The number of messages and bytes sent and received via the interface,
//    since the beginning;
//: o The number of messages per second and bytes per second sent and
//    received over the last 10 seconds.
// So basically we need 4 metrics for input and the same 4 metrics for
// output.
//
// We will create a 'StatContext' to track the data being sent and received
// over our network interface.  To do this, we will tell the StatContext
// to create 2 StatValues; one to keep track of incoming traffic and one
// for outgoing traffic.  We want both values to remember 11 snapshots (10
// seconds of past snapshots plus 1 for the most recent snapshot), so
// we pass this in as the second argument to the 'value()' function of
// 'mwcst::StatContextConfiguration'.  Notice that
// 'mwcst::StatContextConfiguration' methods all return a reference to the
// object, allowing calls to it to be chained as shown.
//..
//  int numSnapshots = 11;
//  mwcst::StatContext context(
//                         mwcst::StatContextConfiguration("Network Interface")
//                                                 .value("In", numSnapshots)
//                                                 .value("Out", numSnapshots),
//                         allocator);
//..
// Now we can use this context in the network interface code to keep
// track of the data that is received and sent.  For example, the code
// below records a message being received.  The first parameter to
// 'adjustValue' indicates the variable we want to update. It is the
// index of that variable in the configuration used to create the
// 'StatContext'. Variables are indexed according to the order in which they
// were added to the 'StatContext.' The second parameter is the delta by which
// the variable is updated.  Basically, we are incrementing the total number
// of bytes received by the length of the current message, and we are letting
// the variable know that we got one more message (one more adjustment). Note
// how the 'valueIndex' values (first argument to 'adjustValue') correspond to
// the order of calls to 'value' above.
//..
//  enum { e_IN = 0, e_OUT = 1 };
//  int messageLength = 100;    // size of the message in bytes
//  context.adjustValue(e_IN, messageLength);
//..
// If we receive another message, we just do the same thing again.
//..
//  messageLength = 200;    // bytes
//  context.adjustValue(e_IN, messageLength);
//..
// Suppose a second has elapsed.  We need to tell the context to snapshot
// its current values and move them into the history of 10 seconds it
// keeps for us.  Here is how to do it:
//..
//  context.snapshot();
//..
// Finally we can print the statistics in tabular format using the code
// below:
//..
//  mwcst::StatContextTableInfoProvider tip;
//  tip.setContext(&context);
//..
// Here we define the columns to show when printing out StatContext.
// Columns are separated into column groups, which share a second-level
// header. First we add the id column and its empty group to show the
// name of our StatContext:
//..
//  tip.setColumnGroup("");
//  tip.addDefaultIdColumn("");
//..
// Now we add the column group for our Input statistics.
//..
//  tip.setColumnGroup("In");
//..
// First, want a column named "Bytes" which contains the value of the '0'th
// (i.e. most recent) snapshot of the e_IN stat context.
//..
//  tip.addColumn("Bytes", e_IN, mwcst::StatUtil::value, 0);
//..
// Next we want column "Bytes/s" which calculates the rate of change of
// the e_IN value from the 10th (oldest) to the 0th (most recent) snapshot.
//..
//  tip.addColumn("Bytes/s", e_IN, mwcst::StatUtil::rate, 10, 0);
//..
// Third, we want a column "Messages" showing the number of times the e_IN
// value has been incremented up to the most recent (0th) snapshot.
//..
//  tip.addColumn("Messages", e_IN, mwcst::StatUtil::increments, 0);
//..
// Finally, we want "Messages/s" which shows the rate of increments of the
// e_IN value from the 10th to the 0th snapshot.
//..
//  tip.addColumn("Messages/s", e_IN, mwcst::StatUtil::incrementRate, 10, 0);
//..
// Now we add similar columns for the output statistics
//..
//  tip.setColumnGroup("Out");
//  tip.addColumn("Bytes", e_OUT, mwcst::StatUtil::value, 0);
//  tip.addColumn("Bytes/s", e_OUT, mwcst::StatUtil::rate, 10, 0);
//  tip.addColumn("Messages", e_OUT, mwcst::StatUtil::increments, 0);
//  tip.addColumn("Messages/s", e_OUT, mwcst::StatUtil::incrementRate, 10, 0);
//..
// Before printing, we tell the 'StatContextTableInfoProvider' to update
// itself, figuring out which StatContexts to display when printing
//..
//  tip.update();
//
//  if (verbose) {
//      mwcst::TableUtil::printTable(bsl::cout, tip);
//  }
//..
// The input stats are printed as follow (output is removed for clarity):
//..
//                   |                  In                 |
//                   | Bytes| Bytes/s| Messages| Messages/s|
//  -----------------+------+--------+---------+-----------+
//  Network Interface|   300|   30.00|        2|       0.20|
//..
// This means that we have received 2 messages for a total of 300 bytes.
// The average number of bytes per second over the last 10 seconds was
// 30 (since only one second has elapsed).  Similarly we are receiving
// 0.2 messages per second.
//
// The following code tests our 'StatContext' with a simulated period of
// 15 seconds, each time calling 'snapshot' to mark the end of a second.
// To verify that the numbers are correct, we update the input value as if
// we received a single message of 100 bytes each second.  We also
// reprint the 'StatContext' after each second elapse.  We start the
// test by calling 'clearValue' to erase any history in the context.
//..
//  context.clearValues();
//  for (int i = 0; i < 15; ++i) {
//      // Receive a message
//      messageLength = 100;    // bytes
//      context.adjustValue(e_IN, messageLength);
//
//      // Process the statistics
//      context.snapshot();
//
//      // Print the table
//      if (verbose) {
//          bsl::cout << bsl::endl << "After " << (i + 1) << " seconds:"
//                    << bsl::endl;
//          tip.update();
//          mwcst::TableUtil::printTable(bsl::cout, tip);
//      }
//  }
//..
// The output of the test will show the byte rate increasing and then
// stabilizing at 100 bytes/s, and similarly the message rate increasing
// and stabilizing at 1 message/s.  The number of bytes and messages
// received keep increasing of course since they are counted since the
// beginning and not over the last 10 seconds.  The last input stats look
// like this:
//
// After 15 seconds:
//..
//                      |                  In                 |
//                      | Bytes| Bytes/s| Messages| Messages/s|
//     -----------------+------+--------+---------+-----------+
//     Network Interface| 1,500|  100.00|       15|       1.00|
//..
/// Using Sub-Tables
///----------------
// This example demonstrates how to use the sub-table feature.  We will
// create a context named "Network Interface" representing a TCP interface
// listening for connections on a given port.  Each time a new client
// connects, we want to keep track of the incoming traffic for the client,
// and we want to aggregate it at the level of "Network Interface".
//
// First we create a StatContext with a single value for incoming traffic,
// similarly to the previous example. We don't create a value for outgoing
// traffic to keep the code minimal. We create the context for the
// "Network Interface" as a table.
//..
//  int numSnapshots = 11;
//  mwcst::StatContext context(
//                         mwcst::StatContextConfiguration("Network Interface")
//                                                  .isTable(true)
//                                                  .value("In", numSnapshots),
//                         allocator);
//..
// Now if we have 2 clients connecting to our network interface, we can
// create a sub-table for each of them.  We give a name to each subtable
// (in this case some URL indicating the remote port).  Subcontexts added
// to a table StatContext are always sub-tables.  The 'isTable' flag and
// any value definitions are ignored when adding a subcontext to a table
// StatContext.
//..
//  bslma::ManagedPtr<mwcst::StatContext> client1 =
//        context.addSubcontext(mwcst::StatContextConfiguration("tcp://1234"));
//  bslma::ManagedPtr<mwcst::StatContext> client2 =
//        context.addSubcontext(mwcst::StatContextConfiguration("tcp://5678"));
//..
// Suppose we receive a mesage from the first client and another message
// from the second client:
//..
//  int messageLength = 100;    // size of the message in bytes
//  client1->adjustValue(0, messageLength);
//
//  messageLength = 300;    // size of the message in bytes
//  client2->adjustValue(0, messageLength);
//..
// Now lets print statistics.  We first call 'snapshot' on the top-level
// context, which cascades to the sub-tables (so we don't need to call
// 'snapshot' on each client).  Printing is done as usual.
//..
//  context.snapshot();
//  mwcst::StatContextTableInfoProvider tip;
//  tip.setContext(&context);
//  tip.setColumnGroup("");
//  tip.addDefaultIdColumn("");
//  tip.setColumnGroup("In");
//  tip.addColumn("Bytes", 0, mwcst::StatUtil::value, 0);
//  tip.addColumn("Bytes/s", 0, mwcst::StatUtil::rate, 10, 0);
//  tip.addColumn("Messages", 0, mwcst::StatUtil::increments, 0);
//  tip.addColumn("Messages/s", 0, mwcst::StatUtil::incrementRate, 10, 0);
//  tip.update();
//
//  if (verbose) {
//      mwcst::TableUtil::printTable(bsl::cout, tip);
//  }
//..
// This will print the hierarchy of contexts as follow.  Note that there
// is a line for the value statistics for "Network Interface" itself which
// is zero because we didn't touch its value.
//..
//                   |                  In
//                   | Bytes| Bytes/s| Messages| Messages/s
//  -----------------+------+--------+---------+-----------
//  Network Interface|   400|   40.00|        2|       0.20
//    *direct*       |     0|    0.00|        0|       0.00
//    tcp://5678     |   300|   30.00|        1|       0.10
//    tcp://1234     |   100|   10.00|        1|       0.10
//..
// Now suppose that "client1" disconnects.  We clear our reference to the
// corresponding sub-table.  When we take another snapshot and reprint the
// top-level context, "client1" is still there but it is marked as deleted.
//..
//  client1.clear();
//  context.snapshot();
//  tip.update();
//  if (verbose) {
//      mwcst::TableUtil::printTable(bsl::cout, tip);
//  }
//..
// Notice the parentheses around the name of deleted client:
//..
//                   |                  In
//                   | Bytes| Bytes/s| Messages| Messages/s
//  -----------------+------+--------+---------+-----------
//  Network Interface|   400|   40.00|        2|       0.20
//    *direct*       |     0|    0.00|        0|       0.00
//    tcp://5678     |   300|   30.00|        1|       0.10
//    (tcp://1234)   |   100|   10.00|        1|       0.10
//..
// The 'cleanup' method is used to remove all deleted contexts from the
// statistics of the parent context.  You need to reset the context in
// the 'StatContextTableInfoProvider' to avoid getting an assertion.
//..
//  context.cleanup();
//  tip.setContext(&context);
//  tip.update();
//  if (verbose) {
//      mwcst::TableUtil::printTable(bsl::cout, tip);
//  }
//..
// The code above will print:
//..
//                   |                  In
//                   | Bytes| Bytes/s| Messages| Messages/s
//  -----------------+------+--------+---------+-----------
//  Network Interface|   400|   40.00|        2|       0.20
//    *direct*       |     0|    0.00|        0|       0.00
//    tcp://5678     |   300|   30.00|        1|       0.10
//..
/// Using Sub-Contexts
///------------------
// This example demonstrates how to use the sub-context feature.  Suppose
// we are keeping track of now much memory we use in our system, and we
// want to keep track of the global allocator and of the allocator used
// in our "Network Interface".
//
// First we create a value, measuring a number of bytes in use.  The stats
// we care about are min, max and average.  Note that for average it is
// necessary to include a period, e.g. a number of snapshots.
//..
//  int numSnapshots = 11;
//..
// Then create a context for the whole system.
//..
//  mwcst::StatContext context(mwcst::StatContextConfiguration("Allocator")
//                                              .value("Memory", numSnapshots),
//                             allocator);
//..
// Now create a sub-context of the context, for the interface allocator.
// Note that the context and any sub-contexts must share the same value
// definitions. If you specify unrelated values to contexts and
// sub-contexts, it will work but the printing of stats will show
//..
//  bslma::ManagedPtr<mwcst::StatContext> subContext = context.addSubcontext(
//                     mwcst::StatContextConfiguration("Interface Allocator")
//                                             .value("Memory", numSnapshots));
//..
// Now lets record some data points.
//..
//  int memInUse = 50000;    // size of the allocator in bytes
//  context.setValue(0, memInUse);
//
//  memInUse = 1500;
//  subContext->setValue(0, memInUse);
//..
// Finally lets snapshot and print the main context.
//..
//  context.snapshot();
//  mwcst::StatContextTableInfoProvider tip;
//  tip.setContext(&context);
//  tip.setColumnGroup("");
//  tip.addDefaultIdColumn("");
//  tip.setColumnGroup("In");
//  tip.addColumn("Bytes", 0, mwcst::StatUtil::value, 0);
//  tip.addColumn("Avg", 0, mwcst::StatUtil::average, 10, 0);
//  tip.addColumn("Min", 0, mwcst::StatUtil::absoluteMin);
//  tip.addColumn("Max", 0, mwcst::StatUtil::absoluteMax);
//  tip.update();
//  if (verbose) {
//      mwcst::TableUtil::printTable(bsl::cout, tip);
//  }
//..
// This is what it prints:
//..
//                       |            Memory
//                       | Bytes |   Avg   | Min|  Max
//  ---------------------+-------+---------+----+-------
//  Allocator            | 50,000|  5000.00|   0| 50,000
//    *direct*           | 50,000|  5000.00|   0| 50,000
//    Interface Allocator|  1,500|   150.00|   0|  1,500
//..

#include <mwcst_statvalue.h>

#include <bdlb_variant.h>
#include <bdlcc_objectpool.h>
#include <bdld_manageddatum.h>
#include <bsl_functional.h>
#include <bsl_list.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_allocatorargt.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_spinlock.h>

namespace BloombergLP {

namespace mwcstm {
class StatContextUpdate;
}

namespace mwcst {

// FORWARD DECLARATIONS
class StatContext;
class StatContextConfiguration;
class StatContextIterator;
class StatContextUserData;

// =================
// class StatContext
// =================

/// A hierarchical container of statistics.
class StatContext {
  public:
    // PUBLIC TYPES
    enum ValueType {
        // Types of values that can be returned by 'get(Int64|Double)Value'

        /// Total stats of all childen, direct and expired values (if
        /// configured to remember expired values)
        DMCST_TOTAL_VALUE,

        /// Total of all subcontexts of this StatContext
        DMCST_ACTIVE_CHILDREN_TOTAL_VALUE,

        /// Stats reported directly to this StatContext only
        DMCST_DIRECT_VALUE,

        /// Total of all subcontexts that have been deleted from this
        /// StatContext.  This is always `0` if this StatContext wasn't
        /// configured to store them.
        DMCST_EXPIRED_VALUE
    };

    /// Callback to be invoked during a snapshot;
    typedef bsl::function<void(const StatContext&)> SnapshotCallback;

  private:
    // PRIVATE TYPES
    struct ValueDefinition {
        bsl::string      d_name;
        bsl::vector<int> d_sizes;
        StatValue::Type  d_type;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(ValueDefinition,
                                       bslma::UsesBslmaAllocator)

        // CREATORS
        ValueDefinition(bslma::Allocator* basicAllocator = 0)
        : d_name(basicAllocator)
        , d_sizes(basicAllocator)
        , d_type(StatValue::DMCST_CONTINUOUS)
        {
        }

        ValueDefinition(const ValueDefinition& other,
                        bslma::Allocator*      basicAllocator = 0)
        : d_name(other.d_name, basicAllocator)
        , d_sizes(other.d_sizes, basicAllocator)
        , d_type(other.d_type)
        {
        }
    };

    typedef bdlb::Variant2<bsl::string, bsls::Types::Int64> Id;
    typedef bsl::unordered_multimap<Id, StatContext*, size_t (*)(const Id&)>
                                                  StatContextMap;
    typedef bsl::unordered_map<int, StatContext*> StatContextIdMap;
    typedef StatContextUserData                   UserData;
    typedef bsl::vector<ValueDefinition>          ValueDefs;
    typedef bsl::shared_ptr<ValueDefs>            ValueDefsPtr;

    typedef bsl::list<StatContext*>     StatContextList;
    typedef bsl::vector<StatContext*>   StatContextVector;
    typedef bsl::vector<StatValue>      ValueVec;
    typedef bslma::ManagedPtr<ValueVec> ValueVecPtr;

    typedef StatContextConfiguration Config;

    typedef bdlcc::ObjectPool<ValueVec>   ValueVecPool;
    typedef bsl::shared_ptr<ValueVecPool> ValueVecPoolPtr;

    typedef bslma::ManagedPtr<bdld::ManagedDatum> ManagedDatumMp;

    // DATA
    Id d_id;  // e.g. name

    // identifier that is distinct from siblings of this context
    int d_uniqueId;

    // the unique identifier to assign to the next added subcontext
    bsl::shared_ptr<bsls::AtomicInt> d_nextSubcontextId_p;

    // `true` if either there are no external references to this context,
    // or this context has been released by its parent, and `false`
    // otherwise
    bsls::AtomicInt d_released;

    bsls::AtomicBool d_isDeleted;
    // No more external references to this
    // context

    bool d_isTable;

    bool d_storeExpiredValues;

    bsl::vector<int> d_defaultHistorySizes;
    // Default history sizes for values
    // without it.  This is only used to
    // forward the default to subcontexts.

    ValueDefsPtr d_valueDefs_p;

    ValueVecPoolPtr d_valueVecPool_p;

    ValueVecPtr d_totalValues_p;

    ValueVecPtr d_activeChildrenTotalValues_p;

    ValueVecPtr d_directValues_p;

    ValueVecPtr d_expiredValues_p;

    StatContextMap d_subcontexts;

    StatContextIdMap d_subcontextsById;

    StatContextVector d_deletedSubcontexts;

    StatContextVector d_newSubcontexts;

    bslmt::Mutex d_newSubcontextsLock;

    bslma::ManagedPtr<UserData> d_userData_p;

    mutable bsls::SpinLock d_managedDatumLock;

    mutable bdld::ManagedDatum d_managedDatum;

    SnapshotCallback d_preSnapshotCallback;

    bsls::Types::Int64 d_numSnapshots;  // Number of times this
                                        // 'StatContext' had
                                        // 'snapshot' called on it

    // holds the update between the last two snapshots (not owned)
    mwcstm::StatContextUpdate* d_update_p;

    // which value fields to collect updates for in `d_update_p`
    unsigned int d_updateValueFieldMask;

    bslma::Allocator* d_statValueAllocator_p;

    bslma::Allocator* d_allocator_p;

    // PRIVATE CLASS METHODS
    static void statContextDeleter(void* context_vp, void*);

    static void datumDeleter(void* datum_vp, void* datumLock_vp);

    // PRIVATE MANIPULATORS

    /// Initialize the specified `vec` using `d_valueDefs_p`
    void initValues(ValueVecPtr& vec, bsls::Types::Int64 initTime = 0);

    /// Delete everything in `d_deletedSubcontexts`
    void clearDeletedSubcontexts(bsl::vector<ValueVec*>* expiredValuesVec);

    /// Move the subcontexts in `d_newSubcontexts` into `d_subcontexts`.
    void moveNewSubcontexts();

    /// Return the value vector containing the `total` values of this
    /// StatContext.
    ValueVec* getTotalValuesVec();

    /// Snapshot the specified `subcontext`
    void snapshotSubcontext(StatContext*       subcontext,
                            bsls::Types::Int64 snapshotTime);

    /// Snapshot all values of all subcontexts.
    void snapshotImp(bsls::Types::Int64 snapshotTime);

    /// Imp of `cleanup`.  Add the direct values of all subcontexts
    /// being deleted to the specified `expiredValuesVec`
    void cleanupImp(bsl::vector<ValueVec*>* expiredValuesVec);

    /// Update the values of this context and all subcontexts based on the
    /// contents of the specified `update`.
    void applyUpdate(const mwcstm::StatContextUpdate& update);

    // NOT IMPLEMENTED
    StatContext(const StatContext&) BSLS_KEYWORD_DELETED;
    StatContext& operator=(const StatContext&) BSLS_KEYWORD_DELETED;

    // FRIENDS
    friend class StatContextConfiguration;
    friend class StatContextIterator;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatContext, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a StatContext with the specified `config`.
    StatContext(const Config& config, bslma::Allocator* basicAllocator = 0);

    /// Destroy this object.
    ~StatContext();

    // MANIPULATORS

    /// Add a subcontext with the specified `config`.  If this
    /// `StatContext` is a table, the `isTable` and `valueDefinitions`
    /// fields  of the `config` are ignored.
    bslma::ManagedPtr<StatContext> addSubcontext(const Config& config);

    /// Adjust the value at the specified index `valueKey` using the
    /// specified `delta`.  Note that this method is thread-safe.  The
    /// behavior is undefined unless the value corresponding to the
    /// `valueKey` is continuous.
    void adjustValue(int valueKey, bsls::Types::Int64 delta);

    /// Set the value at the specified index `valueKey` using the specified
    /// `value`.  Note that this method is thread-safe.  The behavior is
    /// undefined unless the value corresponding to the `valueKey` is
    /// continuous.
    void setValue(int valueKey, bsls::Types::Int64 value);

    /// Set the value at the specified index `valueKey` using the specified
    /// `value`.  Note that this method is thread-safe.  The behavior is
    /// undefined unless the value corresponding to the `valueKey`
    /// is discrete.
    void reportValue(int valueKey, bsls::Types::Int64 value);

    /// Snapshot all values of all subcontexts, then snapshot the user data
    /// associated with this context, if any.
    void snapshot();

    /// Remove any deleted subcontexts of this StatContext and of all
    /// subcontexts.
    void cleanup();

    /// Clear all history of all values.
    void clearValues();

    /// Remove all subcontexts of this StatContext.
    void clearSubcontexts();

    /// Add and remove subcontexts, and update the fields of all values
    /// according to the specified `update`.  The behavior is undefined if
    /// this method is called and the context is configured to record its
    /// snapshots.
    void snapshotFromUpdate(const mwcstm::StatContextUpdate& update);

    // ACCESSORS
    bool hasName() const;
    bool hasId() const;

    /// Return the name assigned to this context.  Undefined unless
    /// `hasName() == true`.
    const bsl::string& name() const;

    /// Return the id assigned to this context.  Undefined unless
    /// `hasId() == true`.
    bsls::Types::Int64 id() const;

    /// Return `true` if this StatContext was created as a table.
    bool isTable() const;

    /// Return `true` if this StatContext has been marked `deleted` meaning
    /// no more external references exist to it.  It will actually be
    /// deleted during its parent's next `cleanup`.
    bool isDeleted() const;

    /// Return `true` if we have any expired StatContexts whose values
    /// we've remembered.
    bool hasExpiredValues() const;

    /// Return `true` if this StatContext was configured with a default
    /// history size that will be forwarded to subcontexts that do not have
    /// one specified and `false` otherwise.
    bool hasDefaultHistorySize() const;

    /// Return the unique Id of this `StatContext`.  Note that a unique Id
    /// is assigned to each new subcontext.  This Id is unique only within
    /// all sibling subcontexts.
    int uniqueId() const;

    /// Return the number of subcontexts held by this `StatContext`
    int numSubcontexts() const;

    /// Return an object that may be used to iterate over all subcontexts
    /// within this object.
    StatContextIterator subcontextIterator() const;

    /// Return a non-modifiable pointer to the subcontext of this context
    /// having the specified `key`, or 0 if no such context exists.  The
    /// behavior is undefined if the returned context is accessed after
    /// having been marked as deleted, and then cleaned up.
    const StatContext* getSubcontext(const bslstl::StringRef& key) const;
    const StatContext* getSubcontext(bsls::Types::Int64 key) const;

    /// Return the number of values
    int numValues() const;

    /// Get the name of the value at the specified `valueIndex`.
    const bsl::string& valueName(int valueIndex) const;

    /// Get the index for the value with the specified `name` or return
    /// `-1` if a value with this `name` isn't a part of this
    /// `StatContext`.
    int valueIndex(const bslstl::StringRef& name) const;

    /// Get the value with the specified `valueIndex` of the specified
    /// `valueType`
    const StatValue& value(ValueType valueType, int valueIndex) const;

    /// Return a modifiable pointer to the user data associated with this
    /// context, or 0 if no such data exists.
    UserData* userData() const;

    /// Return a pointer to the datum allocator in this context.
    bslma::Allocator* datumAllocator() const;

    /// Return a managed pointer to the datum in this context.
    bslma::ManagedPtr<bdld::ManagedDatum> datum() const;

    /// Initialize the specified `update` based on the state of this object.
    /// After initialization, `update` will have a valid configuration, and
    /// a number of empty value upates equal to the number of values within
    /// this context.
    void initializeUpdate(mwcstm::StatContextUpdate* update) const;

    /// Load into the specified `update` the serializable representation of
    /// the latest snapshot of this context.  Optionally specify a
    /// `valueFieldMask` to control which attributes of this value are
    /// loaded into `update`.  If `valueFieldMask` is not specified, all
    /// attributes are saved.  The behavior is undefined if `snapshot` is
    /// called from another thread while this function is executing.
    void loadFullUpdate(mwcstm::StatContextUpdate* update,
                        int valueFieldMask = 0xFFFFFFFF) const;

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
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, const StatContext& context);

// ==============================
// class StatContextConfiguration
// ==============================

/// Configuration for a StatContext
class StatContextConfiguration {
  private:
    // DATA
    StatContext::Id                      d_id;
    int                                  d_uniqueId;
    bsl::vector<int>                     d_defaultHistorySizes;
    StatContext::ValueDefs               d_valueDefs;
    bool                                 d_isTable;
    bsl::shared_ptr<StatContextUserData> d_userData_p;
    bool                                 d_storeExpiredSubcontextValues;
    StatContext::SnapshotCallback        d_preSnapshotCallback;
    const mwcstm::StatContextUpdate*     d_update_p;
    mwcstm::StatContextUpdate*           d_updateCollector_p;
    int                                  d_updateValueFieldMask;
    bsl::shared_ptr<bsls::AtomicInt>     d_nextSubcontextId_p;
    bslma::Allocator*                    d_statValueAllocator_p;

    // FRIENDS
    friend class StatContext;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatContextConfiguration,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a stat context configuration having the specified `name` as
    /// the identity.  Optionally specify a `basicAllocator` used to supply
    /// memory.  If `basicAllocator` is 0, the currently installed default
    /// allocator is used.
    explicit StatContextConfiguration(const bslstl::StringRef& name,
                                      bslma::Allocator* basicAllocator = 0);

    /// Create a stat context configuration having the specified `id` as
    /// the identity.  Optionally specify a `basicAllocator` used to supply
    /// memory.  If `basicAllocator` is 0, the currently installed default
    /// allocator is used.
    explicit StatContextConfiguration(bsls::Types::Int64 id,
                                      bslma::Allocator*  basicAllocator = 0);

    /// Create a stat context configuration having the same value as the
    /// specified `other` object.  Optionally specify a `basicAllocator`
    /// used to supply memory.  If `basicAllocator` is 0, the currently
    /// installed default allocator is used.
    StatContextConfiguration(const StatContextConfiguration& other,
                             bslma::Allocator* basicAllocator = 0);

    /// Create a stat context configuration based on the specified `update`.
    /// Optionally specify a `basicAllocator` used to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.  The behavior is undefined unless 'update.flags() &
    /// mwcstm::StatContextUpdateFlags::DMCSTM_CONTEXT_CREATED' and
    /// `!update.configuration().isNull()`.  The behavior is also undefined
    /// if this configuration conflicts with that of `update`, e.g.,
    /// `isTable` differs, or the values differ.  Note that `update` is held
    /// by reference; the behavior is undefined if `update` is destroy
    /// before this configuration.
    explicit StatContextConfiguration(const mwcstm::StatContextUpdate& update,
                                      bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS
    StatContextConfiguration& isTable(bool value);
    StatContextConfiguration&
    userData(bslma::ManagedPtr<StatContextUserData> userData);

    /// Set the allocator used for creating the StatContext's StatValues
    /// and their containing vectors.  This setting has no effect if
    /// specified for a sub-context.
    StatContextConfiguration& statValueAllocator(bslma::Allocator* allocator);

    /// Set whether the stat context should aggregate the final values of
    /// expired sub contexts.  This will cause the statContext's total
    /// values to include everything since it was created.  Return this
    /// object.
    StatContextConfiguration& storeExpiredSubcontextValues(bool value);

    /// Configure the default history size of this stat context with the
    /// specified `level1` and optionally specified `level2` and `level3`
    /// snapshot history sizes.  This history size configuration will be
    /// used for values that aren't defined with a history size.  This
    /// default configuration will also be inherited by any subcontexts
    /// that don't override it.
    StatContextConfiguration& defaultHistorySize(int level1);
    StatContextConfiguration& defaultHistorySize(int level1, int level2);
    StatContextConfiguration&
    defaultHistorySize(int level1, int level2, int level3);

    /// Add a StatValue of the optionally specified `type` to the
    /// StatContext with the specified `name` and optionally specified
    /// `historySize`, . If the `historySize` isn't provided, use the
    /// default history size configuration of the stat context.  If the
    /// `type` isn't provided, assume the value is continuous.  Return this
    /// object. Note that the `valueIndex` of the added StatValue will be
    /// one plus the `valueIndex` of the previously added StatValue, or
    /// zero if this is the first StatValue added.
    StatContextConfiguration& value(const bslstl::StringRef& name);
    StatContextConfiguration& value(const bslstl::StringRef& name,
                                    StatValue::Type          type);
    StatContextConfiguration& value(const bslstl::StringRef& name,
                                    int                      historySize);
    StatContextConfiguration& value(const bslstl::StringRef& name,
                                    StatValue::Type          type,
                                    int                      historySize);

    /// Add a snapshot level of the specified `size` to the last added
    /// value.  A snapshot of a given level represents a range of snapshots
    /// of a lower level.  For example, if a value was added with
    /// a `historySize` of `60`, and then a valueLevel was added with
    /// a `size` of `5`, then every 60 snapshots, they will all be
    /// combined into a single aggregated snapshot.  This allows the
    /// StatValue to approximately maintain 60*5 snapshots worth of
    /// history, but only use 60+5 snapshots worth of memory.  The behavior
    /// is undefined if the last value wasn't created with a history
    /// size.
    StatContextConfiguration& valueLevel(int size);

    /// Set a callback to be invoked right before the `StatContext` is
    /// snapshotted.  Return this object.
    StatContextConfiguration& preSnapshotCallback(
        const StatContext::SnapshotCallback& preSnapshotCallback);

    /// Configure the stat context to update the specified `update` with
    /// the changed values of the context and any subcontext each time
    /// `snapshot` is called.  Optionally specify a `valueFieldMask` to
    /// control which changed fields of a `StatValue` are recorded.
    /// If `valueFieldMask` is not specified, all fields are recorded.
    /// A field is recorded if it has changed since the last snapshot, and
    /// `valueFieldMask & (1 << mwcst::StatValueField::FIELD)`.  Note that
    /// if this configuration is used to create a subcontext of a
    /// `StatContext` that already is collecting updates, this option is
    /// ignored.
    StatContextConfiguration&
    enableUpdateCollection(mwcstm::StatContextUpdate* update,
                           int valueFieldMask = 0xFFFFFFFF);
};

// =========================
// class StatContextIterator
// =========================

/// Iterator over the active and deleted subcontexts of a StatContext.
class StatContextIterator {
  private:
    // PRIVATE TYPES
    typedef StatContext::StatContextMap    StatContextMap;
    typedef StatContext::StatContextVector StatContextVector;

    // DATA
    StatContextMap::const_iterator    d_mapIter;
    StatContextMap::const_iterator    d_mapEnd;
    StatContextVector::const_iterator d_listIter;
    StatContextVector::const_iterator d_listEnd;

    // PRIVATE CREATORS
    StatContextIterator(StatContextMap::const_iterator    mapBegin,
                        StatContextMap::const_iterator    mapEnd,
                        StatContextVector::const_iterator listBegin,
                        StatContextVector::const_iterator listEnd);

    // FRIENDS
    friend class StatContext;

  public:
    // CREATORS
    StatContextIterator(const StatContextIterator& other);

    // MANIPULATORS
    StatContextIterator& operator=(const StatContextIterator& rhs);
    bool                 operator++();

    // ACCESSORS
    const StatContext* operator->() const;
    const StatContext& operator*() const;
                       operator const StatContext*() const;
                       operator bool() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class StatContext
// -----------------

// CREATORS
inline StatContext::~StatContext()
{
    clearSubcontexts();
}

// MANIPULATORS
inline void StatContext::adjustValue(int valueKey, bsls::Types::Int64 delta)
{
    BSLS_ASSERT(valueKey < static_cast<int>(d_directValues_p->size()));
    (*d_directValues_p)[valueKey].adjustValue(delta);
}

inline void StatContext::setValue(int valueKey, bsls::Types::Int64 value)
{
    BSLS_ASSERT(valueKey < static_cast<int>(d_directValues_p->size()));
    (*d_directValues_p)[valueKey].setValue(value);
}

inline void StatContext::reportValue(int valueKey, bsls::Types::Int64 value)
{
    BSLS_ASSERT(valueKey < static_cast<int>(d_directValues_p->size()));
    (*d_directValues_p)[valueKey].reportValue(value);
}

// ACCESSORS
inline bool StatContext::hasName() const
{
    return d_id.is<bsl::string>();
}

inline bool StatContext::hasId() const
{
    return d_id.is<bsls::Types::Int64>();
}

inline const bsl::string& StatContext::name() const
{
    BSLS_ASSERT(hasName());
    return d_id.the<bsl::string>();
}

inline bsls::Types::Int64 StatContext::id() const
{
    BSLS_ASSERT(hasId());
    return d_id.the<bsls::Types::Int64>();
}

inline bool StatContext::isTable() const
{
    return d_isTable;
}

inline bool StatContext::isDeleted() const
{
    return d_isDeleted;
}

inline bool StatContext::hasExpiredValues() const
{
    return d_expiredValues_p.ptr();
}

inline bool StatContext::hasDefaultHistorySize() const
{
    return !d_defaultHistorySizes.empty();
}

inline int StatContext::uniqueId() const
{
    return d_uniqueId;
}

inline int StatContext::numSubcontexts() const
{
    return static_cast<int>(d_subcontexts.size() +
                            d_deletedSubcontexts.size());
}

inline StatContextIterator StatContext::subcontextIterator() const
{
    return StatContextIterator(d_subcontexts.begin(),
                               d_subcontexts.end(),
                               d_deletedSubcontexts.begin(),
                               d_deletedSubcontexts.end());
}

inline const StatContext*
StatContext::getSubcontext(const bslstl::StringRef& key) const
{
    StatContextMap::const_iterator iter = d_subcontexts.find(
        Id(bsl::string(key)));
    return iter == d_subcontexts.end() ? 0 : iter->second;
}

inline const StatContext*
StatContext::getSubcontext(bsls::Types::Int64 key) const
{
    StatContextMap::const_iterator iter = d_subcontexts.find(Id(key));
    return iter == d_subcontexts.end() ? 0 : iter->second;
}

inline int StatContext::numValues() const
{
    return d_valueDefs_p ? static_cast<int>(d_valueDefs_p->size()) : 0;
}

inline const bsl::string& StatContext::valueName(int valueIndex) const
{
    BSLS_ASSERT(valueIndex < static_cast<int>(d_valueDefs_p->size()));
    return (*d_valueDefs_p)[valueIndex].d_name;
}

inline const StatValue& StatContext::value(ValueType valueType,
                                           int       valueIndex) const
{
    const ValueVec* valueVec = 0;
    switch (valueType) {
    case DMCST_TOTAL_VALUE:
        valueVec = d_totalValues_p.ptr() ? d_totalValues_p.ptr()
                                         : d_directValues_p.ptr();
        break;
    case DMCST_ACTIVE_CHILDREN_TOTAL_VALUE:
        valueVec = d_activeChildrenTotalValues_p.ptr();
        break;
    case DMCST_DIRECT_VALUE: valueVec = d_directValues_p.ptr(); break;
    case DMCST_EXPIRED_VALUE: valueVec = d_expiredValues_p.ptr(); break;
    }

    return (*valueVec)[valueIndex];
}

inline StatContextUserData* StatContext::userData() const
{
    return d_userData_p.ptr();
}

inline bslma::Allocator* StatContext::datumAllocator() const
{
    return d_managedDatum.allocator();
}

inline bslma::ManagedPtr<bdld::ManagedDatum> StatContext::datum() const
{
    d_managedDatumLock.lock();
    return bslma::ManagedPtr<bdld::ManagedDatum>(&d_managedDatum,
                                                 &d_managedDatumLock,
                                                 &StatContext::datumDeleter);
}

// ------------------------------
// class StatContextConfiguration
// ------------------------------

// CREATORS
inline StatContextConfiguration::StatContextConfiguration(
    const bslstl::StringRef& name,
    bslma::Allocator*        basicAllocator)
: d_id(bsl::string(name), basicAllocator)
, d_uniqueId(0)
, d_defaultHistorySizes(basicAllocator)
, d_valueDefs(basicAllocator)
, d_isTable(false)
, d_userData_p()
, d_storeExpiredSubcontextValues(false)
, d_preSnapshotCallback(bsl::allocator_arg, basicAllocator)
, d_update_p(0)
, d_updateCollector_p(0)
, d_updateValueFieldMask(0)
, d_nextSubcontextId_p()
, d_statValueAllocator_p(0)
{
}

inline StatContextConfiguration::StatContextConfiguration(
    bsls::Types::Int64 id,
    bslma::Allocator*  basicAllocator)
: d_id(id, basicAllocator)
, d_uniqueId(0)
, d_defaultHistorySizes(basicAllocator)
, d_valueDefs(basicAllocator)
, d_isTable(false)
, d_userData_p()
, d_storeExpiredSubcontextValues(false)
, d_preSnapshotCallback(bsl::allocator_arg, basicAllocator)
, d_update_p(0)
, d_updateCollector_p(0)
, d_updateValueFieldMask(0)
, d_nextSubcontextId_p()
, d_statValueAllocator_p(0)
{
}

inline StatContextConfiguration::StatContextConfiguration(
    const StatContextConfiguration& other,
    bslma::Allocator*               basicAllocator)
: d_id(other.d_id, basicAllocator)
, d_uniqueId(other.d_uniqueId)
, d_defaultHistorySizes(other.d_defaultHistorySizes, basicAllocator)
, d_valueDefs(other.d_valueDefs, basicAllocator)
, d_isTable(other.d_isTable)
, d_userData_p(other.d_userData_p)
, d_storeExpiredSubcontextValues(other.d_storeExpiredSubcontextValues)
, d_preSnapshotCallback(bsl::allocator_arg,
                        basicAllocator,
                        other.d_preSnapshotCallback)
, d_update_p(other.d_update_p)
, d_updateCollector_p(other.d_updateCollector_p)
, d_updateValueFieldMask(other.d_updateValueFieldMask)
, d_nextSubcontextId_p(other.d_nextSubcontextId_p)
, d_statValueAllocator_p(0)
{
}

// MANIPULATORS
inline StatContextConfiguration& StatContextConfiguration::isTable(bool value)
{
    d_isTable = value;
    return *this;
}

inline StatContextConfiguration& StatContextConfiguration::userData(
    bslma::ManagedPtr<StatContextUserData> userData)
{
    d_userData_p = userData;
    return *this;
}

inline StatContextConfiguration&
StatContextConfiguration::statValueAllocator(bslma::Allocator* allocator)
{
    d_statValueAllocator_p = allocator;
    return *this;
}

inline StatContextConfiguration&
StatContextConfiguration::storeExpiredSubcontextValues(bool value)
{
    d_storeExpiredSubcontextValues = value;
    return *this;
}

inline StatContextConfiguration&
StatContextConfiguration::defaultHistorySize(int level1)
{
    d_defaultHistorySizes.clear();
    d_defaultHistorySizes.push_back(level1);
    return *this;
}

inline StatContextConfiguration&
StatContextConfiguration::defaultHistorySize(int level1, int level2)
{
    d_defaultHistorySizes.clear();
    d_defaultHistorySizes.push_back(level1);
    d_defaultHistorySizes.push_back(level2);
    return *this;
}

inline StatContextConfiguration&
StatContextConfiguration::defaultHistorySize(int level1,
                                             int level2,
                                             int level3)
{
    d_defaultHistorySizes.clear();
    d_defaultHistorySizes.push_back(level1);
    d_defaultHistorySizes.push_back(level2);
    d_defaultHistorySizes.push_back(level3);
    return *this;
}

inline StatContextConfiguration&
StatContextConfiguration::value(const bslstl::StringRef& name)
{
    return value(name, StatValue::DMCST_CONTINUOUS);
}

inline StatContextConfiguration&
StatContextConfiguration::value(const bslstl::StringRef& name,
                                StatValue::Type          type)
{
    d_valueDefs.resize(d_valueDefs.size() + 1);
    d_valueDefs.back().d_name = name;
    d_valueDefs.back().d_type = type;
    return *this;
}

inline StatContextConfiguration&
StatContextConfiguration::value(const bslstl::StringRef& name, int historySize)
{
    return value(name, StatValue::DMCST_CONTINUOUS, historySize);
}

inline StatContextConfiguration&
StatContextConfiguration::value(const bslstl::StringRef& name,
                                StatValue::Type          type,
                                int                      historySize)
{
    d_valueDefs.resize(d_valueDefs.size() + 1);
    d_valueDefs.back().d_name = name;
    d_valueDefs.back().d_type = type;
    d_valueDefs.back().d_sizes.push_back(historySize);
    return *this;
}

inline StatContextConfiguration& StatContextConfiguration::valueLevel(int size)
{
    BSLS_ASSERT(!d_valueDefs.empty() && !d_valueDefs.back().d_sizes.empty());
    d_valueDefs.back().d_sizes.push_back(size);
    return *this;
}

inline StatContextConfiguration& StatContextConfiguration::preSnapshotCallback(
    const StatContext::SnapshotCallback& preSnapshotCallback)
{
    d_preSnapshotCallback = preSnapshotCallback;
    return *this;
}

inline StatContextConfiguration&
StatContextConfiguration::enableUpdateCollection(
    mwcstm::StatContextUpdate* update,
    int                        valueFieldMask)
{
    d_updateCollector_p    = update;
    d_updateValueFieldMask = valueFieldMask;
    return *this;
}

// -------------------------
// class StatContextIterator
// -------------------------

// PRIVATE CREATORS
inline StatContextIterator::StatContextIterator(
    StatContextMap::const_iterator    mapBegin,
    StatContextMap::const_iterator    mapEnd,
    StatContextVector::const_iterator listBegin,
    StatContextVector::const_iterator listEnd)
: d_mapIter(mapBegin)
, d_mapEnd(mapEnd)
, d_listIter(listBegin)
, d_listEnd(listEnd)
{
}

// CREATORS
inline StatContextIterator::StatContextIterator(
    const StatContextIterator& other)
: d_mapIter(other.d_mapIter)
, d_mapEnd(other.d_mapEnd)
, d_listIter(other.d_listIter)
, d_listEnd(other.d_listEnd)
{
}

// MANIPULATORS
inline StatContextIterator&
StatContextIterator::operator=(const StatContextIterator& rhs)
{
    d_mapIter  = rhs.d_mapIter;
    d_mapEnd   = rhs.d_mapEnd;
    d_listIter = rhs.d_listIter;
    d_listEnd  = rhs.d_listEnd;

    return *this;
}

inline bool StatContextIterator::operator++()
{
    if (d_mapIter != d_mapEnd) {
        ++d_mapIter;
    }
    else if (d_listIter != d_listEnd) {
        ++d_listIter;
    }
    else {
        BSLS_ASSERT(false);
    }

    return *this;
}

// ACCESSORS
inline const StatContext* StatContextIterator::operator->() const
{
    if (d_mapIter != d_mapEnd) {
        return d_mapIter->second;
    }
    else {
        return *d_listIter;
    }
}

inline const StatContext& StatContextIterator::operator*() const
{
    if (d_mapIter != d_mapEnd) {
        return *d_mapIter->second;
    }
    else {
        return **d_listIter;
    }
}

inline StatContextIterator::operator const StatContext*() const
{
    if (d_mapIter != d_mapEnd) {
        return d_mapIter->second;
    }
    else {
        return *d_listIter;
    }
}

inline StatContextIterator::operator bool() const
{
    return d_mapIter != d_mapEnd || d_listIter != d_listEnd;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mwcst::operator<<(bsl::ostream&      stream,
                                       const StatContext& context)
{
    return context.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
