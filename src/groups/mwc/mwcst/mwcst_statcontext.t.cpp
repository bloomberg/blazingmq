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

// mwcst_statcontext.t.cpp                                            -*-C++-*-

// Component under test
#include <mwcst_printutil.h>
#include <mwcst_statcontext.h>
#include <mwcst_statcontexttableinfoprovider.h>
#include <mwcst_statutil.h>
#include <mwcst_tableutil.h>
#include <mwcstm_values.h>

// MWC
#include <mwcst_testutil.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_bitutil.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bslma_testallocator.h>
#include <bslma_testallocatormonitor.h>

using namespace BloombergLP;
using namespace bsl;
using namespace mwcst;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
// The component under test is a mechanism used to keep track of a hierarchical
// set of user-defined statistics (e.g. data points).
//
// The test plan is limited to usage examples for the standard features, as
// well as test cases for the update feature which allows serializing a stat
// context.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// [ 1] Usage example
// [ 2] Usage example with tables
// [ 3] Usage example with subcontext
// [ 4] Usage example with value level
// [ 4] Test updates
// [ 5] Usage examples with updates
//-----------------------------------------------------------------------------

//=============================================================================
//                  STANDARD BDE ASSERT TEST MACROS
//-----------------------------------------------------------------------------

namespace {

static int  testStatus          = 0;
static bool verbose             = 0;
static bool veryVerbose         = 0;
static bool veryVeryVerbose     = 0;
static bool veryVeryVeryVerbose = 0;

static void aSsErT(int c, const char* s, int i)
{
    if (c) {
        cout << "Error " << __FILE__ << "(" << i << "): " << s
             << "    (failed)" << endl;
        if (0 <= testStatus && testStatus <= 100)
            ++testStatus;
    }
}

}  // close anonymous namespace

//=============================================================================
//                       STANDARD BDE TEST DRIVER MACROS
//-----------------------------------------------------------------------------

#define ASSERT BSLS_BSLTESTUTIL_ASSERT

// ============================================================================
//                             USEFUL MACROS
// ----------------------------------------------------------------------------

// The following macros may be used to print an expression 'X' at different
// levels of verbosity.  Note that 'X' is not surrounded with parentheses so
// that expressions containing output stream operations can be supported.

#define PV(X)                                                                 \
    if (verbose)                                                              \
        cout << endl << X << endl;

//=============================================================================
//               GLOBAL HELPER CLASSES AND FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

static bsls::Types::Int64 MAX_INT =
    bsl::numeric_limits<bsls::Types::Int64>::max();
static bsls::Types::Int64 MIN_INT =
    bsl::numeric_limits<bsls::Types::Int64>::min();

/// Compare the snapshot of the specified `value` at he specified `index`
/// of the specified `level` to the specified `snapshotDesc`.
/// `snapshotDesc` is a space separated list of integeres of the form
/// "<value> <min> <max> <increments> <decrements>" if `value` is
/// continuous or
/// "<min> <max> <events> <sum>" if `value` is discrete
static bool checkSnapshot(const StatValue& value,
                          int              level,
                          int              index,
                          const char*      snapshotDesc)
{
    const StatValue::Snapshot& snapshot = value.snapshot(
        StatValue::SnapshotLocation(level, index));

    bsl::vector<bsls::Types::Int64> snapshotValues;
    if (value.type() == StatValue::DMCST_CONTINUOUS) {
        snapshotValues.push_back(snapshot.value());
        snapshotValues.push_back(snapshot.min());
        snapshotValues.push_back(snapshot.max());
        snapshotValues.push_back(snapshot.increments());
        snapshotValues.push_back(snapshot.decrements());
    }
    else {
        snapshotValues.push_back(snapshot.min());
        snapshotValues.push_back(snapshot.max());
        snapshotValues.push_back(snapshot.events());
        snapshotValues.push_back(snapshot.sum());
    }

    bsl::vector<bsls::Types::Int64> expectedValues =
        mwcu::TestUtil::int64Vector(snapshotDesc);

    ASSERT_EQUALS(expectedValues, snapshotValues);
    return expectedValues == snapshotValues;
}

/// Return `true` if the specified `update` contains the specified `field`,
/// and `false` otherwise.
static bool hasField(const mwcstm::StatContextUpdate& contextUpdate,
                     int                              index,
                     mwcstm::StatValueFields::Value   field)
{
    const mwcstm::StatValueUpdate& update =
        contextUpdate.directValues()[index];
    return bdlb::BitUtil::isBitSet(update.fieldMask(), field);
}

/// Return the value of the specified `field` within the specified `update`.
/// The behavior is undefined unless `hasField(update, field)`
static bsls::Types::Int64 field(const mwcstm::StatContextUpdate& contextUpdate,
                                int                              index,
                                mwcstm::StatValueFields::Value   field)
{
    ASSERT(hasField(contextUpdate, index, field));
    const mwcstm::StatValueUpdate& update =
        contextUpdate.directValues()[index];
    ASSERT(bdlb::BitUtil::numBitsSet(update.fieldMask()) ==
           static_cast<int>(update.fields().size()));
    int i = bdlb::BitUtil::numBitsSet(update.fieldMask() & ((1 << field) - 1));
    return update.fields()[i];
}

static const StatValue& direct(const StatContext& context, int index)
{
    return context.value(StatContext::DMCST_DIRECT_VALUE, index);
}

//=============================================================================
//                              TEST CASES
//-----------------------------------------------------------------------------

// USAGE EXAMPLES

static void usageExample(bsl::ostream& stream, bslma::Allocator* allocator)
{
    // ------------------------------------------------------------------------
    // USAGE EXAMPLE
    //
    // Concerns:
    //   Test the usage example provided in the component header file.
    // ------------------------------------------------------------------------

    // In this example we will be keeping track of the volumes of data moving
    // in and out of a particular network interface.  The data we want to keep
    // track of is the current real time volumes.  Specifically we want to keep
    // track of:
    // - The number of messages and bytes sent and received via the interface,
    //   since the beginning;
    // - The number of messages per second and bytes per second sent and
    //   received over the last 10 seconds.
    // So basically we need 4 metrics for input and the same 4 metrics for
    // output.
    //
    // We will create a 'StatContext' to track the data being sent and received
    // over our network interface.  To do this, we will tell the StatContext
    // to create 2 StatValues; one to keep track of incoming traffic and one
    // for outgoing traffic.  We want both values to remember 10 snapshots, so
    // we pass this in as the second argument to the 'value()' function of
    // 'mwcst::StatContextConfiguration'.  Notice that
    // 'mwcst::StatContextConfiguration' methods all return a reference to the
    // object, allowing calls to it to be chained as shown.
    int                numSnapshots = 11;
    mwcst::StatContext context(
        mwcst::StatContextConfiguration("Network Interface")
            .value("In", numSnapshots)
            .value("Out", numSnapshots),
        allocator);

    // Now we can use this context in the network interface code to keep
    // track of the data that is received and sent.  For example, the code
    // below records a message being received.  The first parameter to
    // 'adjustValue' indicate the variable we want to update, which is the
    // index of that variable in the configuration used to create the
    // 'StatContext'.  The second parameter is the delta for updating the
    // variable.  Basically we are incrementing the total number of bytes
    // received by the length of the message, and we are letting the variable
    // know that we got one more message (one more adjustment).
    enum { e_IN = 0, e_OUT = 1 };
    int messageLength = 100;  // size of the message in bytes
    context.adjustValue(e_IN, messageLength);

    // If we receive another message, we just do the same thing again.
    messageLength = 200;  // bytes
    context.adjustValue(e_IN, messageLength);

    // Suppose a second has elapsed.  We need to tell the context to snapshot
    // its current values and move them into the history of 10 seconds it
    // keeps for us.  Here is how to do it:
    context.snapshot();

    // Finally we can print the statistics in tabular format using the code
    // below:
    mwcst::StatContextTableInfoProvider tip;
    tip.setContext(&context);

    // Here we define the columns to show when printing out StatContext.
    // Columns are separated into column groups, which share a second-level
    // header. First we add the id column and its empty group to show the
    // name of our StatContext:
    tip.setColumnGroup("");
    tip.addDefaultIdColumn("");

    // Now we add the column group for our Input statistics.
    tip.setColumnGroup("In");

    // First, want a column named "Bytes" which contains the value of the '0'th
    // (i.e. most recent) snapshot of the IN stat context.
    tip.addColumn("Bytes", e_IN, mwcst::StatUtil::value, 0);

    // Next we want column "Bytes/s" which calculates the rate of change of
    // the IN value from the 10th (oldest) to the 0th (most recent) snapshot.
    tip.addColumn("Bytes/s", e_IN, mwcst::StatUtil::rate, 10, 0);

    // Third, we want a column "Messages" showing the number of times the IN
    // value has been incremented up to the most recent (0th) snapshot.
    tip.addColumn("Messages", e_IN, mwcst::StatUtil::increments, 0);

    // Finally, we want "Messages/s" which shows the rate of increments of the
    // IN value from the 10th to the 0th snapshot.
    tip.addColumn("Messages/s", e_IN, mwcst::StatUtil::incrementRate, 10, 0);

    // Now we add similar columns for the output statistics
    tip.setColumnGroup("Out");
    tip.addColumn("Bytes", e_OUT, mwcst::StatUtil::value, 0);
    tip.addColumn("Bytes/s", e_OUT, mwcst::StatUtil::rate, 10, 0);
    tip.addColumn("Messages", e_OUT, mwcst::StatUtil::increments, 0);
    tip.addColumn("Messages/s", e_OUT, mwcst::StatUtil::incrementRate, 10, 0);

    // Before printing, we tell the 'StatContextTableInfoProvider' to update
    // itself, figuring out which StatContexts to display when printing
    tip.update();

    if (verbose) {
        mwcu::TableUtil::printTable(stream, tip);
    }

    // The input stats are printed as follow (output is removed for clarity):
    //
    //                  |                  In                 |
    //                  | Bytes| Bytes/s| Messages| Messages/s|
    // -----------------+------+--------+---------+-----------+
    // Network Interface|   300|   30.00|        2|       0.20|
    //
    // This means that we have received 2 messages for a total of 300 bytes.
    // The average number of bytes per second over the last 10 seconds was
    // 30 (since only one second has elapsed).  Similarly we are receiving
    // 0.2 messages per second.

    // The following code tests our 'StatContext' with a simulated period of
    // 15 seconds, each time calling 'snapshot' to mark the end of a second.
    // To verify that the numbers are correct, we update the input value as if
    // we received a single message of 100 bytes each second.  We also
    // reprint the 'StatContext' after each second elapse.  We start the
    // test by calling 'clearValue' to erase any history in the context.
    context.clearValues();
    for (int i = 0; i < 15; ++i) {
        // Receive a message
        messageLength = 100;  // bytes
        context.adjustValue(e_IN, messageLength);

        // Process the statistics
        context.snapshot();

        // Print the table
        if (verbose) {
            stream << bsl::endl
                   << "After " << (i + 1) << " seconds:" << bsl::endl;
            tip.update();
            mwcu::TableUtil::printTable(stream, tip);
        }
    }

    // The output of the test will show the byte rate increasing and then
    // stabilizing at 100 bytes/s, and similarly the message rate increasing
    // and stabilizing at 1 message/s.  The number of bytes and messages
    // received keep increasing of course since they are counted since the
    // beginning and not over the last 10 seconds.  The last input stats look
    // like this:
    //
    // After 15 seconds:
    //
    //                      |                  In                 |
    //                      | Bytes| Bytes/s| Messages| Messages/s|
    //     -----------------+------+--------+---------+-----------+
    //     Network Interface| 1,500|  100.00|       15|       1.00|
}

static void usageExampleUpdate(bsl::ostream&     stream,
                               bslma::Allocator* allocator)
{
    int                       numSnapshots = 11;
    mwcstm::StatContextUpdate update;
    mwcst::StatContext        context(
        mwcst::StatContextConfiguration("Network Interface")
            .value("In", numSnapshots)
            .value("Out", numSnapshots)
            .enableUpdateCollection(&update),
        allocator);

    enum { e_IN = 0, e_OUT = 1 };
    int messageLength = 100;  // size of the message in bytes
    context.adjustValue(e_IN, messageLength);

    messageLength = 200;  // bytes
    context.adjustValue(e_IN, messageLength);

    context.snapshot();

    // Create a second context whose values and configuration derive from
    // 'update', driven by 'context'.

    mwcst::StatContext updatedContext(
        mwcst::StatContextConfiguration(update, allocator));

    mwcst::StatContextTableInfoProvider tip;
    tip.setContext(&updatedContext);

    tip.setColumnGroup("");
    tip.addDefaultIdColumn("");
    tip.setColumnGroup("In");
    tip.addColumn("Bytes", e_IN, mwcst::StatUtil::value, 0);
    tip.addColumn("Bytes/s", e_IN, mwcst::StatUtil::rate, 10, 0);
    tip.addColumn("Messages", e_IN, mwcst::StatUtil::increments, 0);
    tip.addColumn("Messages/s", e_IN, mwcst::StatUtil::incrementRate, 10, 0);

    tip.setColumnGroup("Out");
    tip.addColumn("Bytes", e_OUT, mwcst::StatUtil::value, 0);
    tip.addColumn("Bytes/s", e_OUT, mwcst::StatUtil::rate, 10, 0);
    tip.addColumn("Messages", e_OUT, mwcst::StatUtil::increments, 0);
    tip.addColumn("Messages/s", e_OUT, mwcst::StatUtil::incrementRate, 10, 0);

    tip.update();

    if (verbose) {
        mwcu::TableUtil::printTable(stream, tip);
    }

    context.clearValues();
    updatedContext.clearValues();
    for (int i = 0; i < 15; ++i) {
        // Receive a message
        messageLength = 100;  // bytes
        context.adjustValue(e_IN, messageLength);

        // Process the statistics
        context.snapshot();
        updatedContext.snapshotFromUpdate(update);

        // Print the table
        if (verbose) {
            stream << bsl::endl
                   << "After " << (i + 1) << " seconds:" << bsl::endl;
            tip.update();
            mwcu::TableUtil::printTable(stream, tip);
        }
    }
}

static void tableUsageExample(bsl::ostream&     stream,
                              bslma::Allocator* allocator)
{
    // ------------------------------------------------------------------------
    // USAGE EXAMPLE WITH TABLE
    //
    // Concerns:
    //   Test the usage example provided in the component header file.
    // ------------------------------------------------------------------------

    // This example demonstrates how to use the sub-table feature.  We will
    // create a context named "Network Interface" representing a TCP interface
    // listening for connections on a given port.  Each time a new client
    // connects, we want to keep track of the incoming traffic for the client,
    // and we want to aggregate it at the level of "Network Interface".

    // First we create a StatContext with a single value for incoming traffic,
    // similarly to the previous example. We don't create a value for outgoing
    // traffic to keep the code minimal. We create the context for the
    // "Network Interface" as a table.
    int                numSnapshots = 11;
    mwcst::StatContext context(
        mwcst::StatContextConfiguration("Network Interface")
            .isTable(true)
            .value("In", numSnapshots),
        allocator);

    // Now if we have 2 clients connecting to our network interface, we can
    // create a sub-table for each of them.  We give a name to each subtable
    // (in this case some URL indicating the remote port).  Subcontexts added
    // to a table StatContext are always sub-tables.  The 'isTable' flag and
    // any value definitions are ignored when adding a subcontext to a table
    // StatContext.
    bslma::ManagedPtr<mwcst::StatContext> client1 = context.addSubcontext(
        mwcst::StatContextConfiguration("tcp://1234"));
    bslma::ManagedPtr<mwcst::StatContext> client2 = context.addSubcontext(
        mwcst::StatContextConfiguration("tcp://5678"));

    // Suppose we receive a mesage from the first client and another message
    // from the second client:
    int messageLength = 100;  // size of the message in bytes
    client1->adjustValue(0, messageLength);

    messageLength = 300;  // size of the message in bytes
    client2->adjustValue(0, messageLength);

    // Now lets print statistics.  We first call 'snapshot' on the top-level
    // context, which cascades to the sub-tables (so we don't need to call
    // 'snapshot' on each client).  Printing is done as usual.
    context.snapshot();
    mwcst::StatContextTableInfoProvider tip;
    tip.setContext(&context);
    tip.setColumnGroup("");
    tip.addDefaultIdColumn("");
    tip.setColumnGroup("In");
    tip.addColumn("Bytes", 0, mwcst::StatUtil::value, 0);
    tip.addColumn("Bytes/s", 0, mwcst::StatUtil::rate, 10, 0);
    tip.addColumn("Messages", 0, mwcst::StatUtil::increments, 0);
    tip.addColumn("Messages/s", 0, mwcst::StatUtil::incrementRate, 10, 0);
    tip.update();

    if (verbose) {
        mwcu::TableUtil::printTable(stream, tip);
    }

    // This will print the hierarchy of contexts as follow.  Note that there
    // is a line for the value statistics for "Network Interface" itself which
    // is zero because we didn't touch its value.
    //
    //                  |                  In
    //                  | Bytes| Bytes/s| Messages| Messages/s
    // -----------------+------+--------+---------+-----------
    // Network Interface|   400|   40.00|        2|       0.20
    //   *direct*       |     0|    0.00|        0|       0.00
    //   tcp://5678     |   300|   30.00|        1|       0.10
    //   tcp://1234     |   100|   10.00|        1|       0.10

    // Now suppose that "client1" disconnects.  We clear our reference to the
    // corresponding sub-table.  When we take another snapshot and reprint the
    // top-level context, "client1" is still there but it is marked as deleted.
    client1.clear();
    context.snapshot();
    tip.update();
    if (verbose) {
        mwcu::TableUtil::printTable(stream, tip);
    }

    // Notice the parentheses around the name of deleted client:
    //
    //                  |                  In
    //                  | Bytes| Bytes/s| Messages| Messages/s
    // -----------------+------+--------+---------+-----------
    // Network Interface|   400|   40.00|        2|       0.20
    //   *direct*       |     0|    0.00|        0|       0.00
    //   tcp://5678     |   300|   30.00|        1|       0.10
    //   (tcp://1234)   |   100|   10.00|        1|       0.10

    // The 'cleanup' method is used to remove all deleted contexts from the
    // statistics of the parent context.  You need to reset the context in
    // the 'StatContextTableInfoProvider' to avoid getting an assertion.
    context.cleanup();
    tip.setContext(&context);
    tip.update();
    if (verbose) {
        mwcu::TableUtil::printTable(stream, tip);
    }

    // The code above will print:
    //
    //                  |                  In
    //                  | Bytes| Bytes/s| Messages| Messages/s
    // -----------------+------+--------+---------+-----------
    // Network Interface|   400|   40.00|        2|       0.20
    //   *direct*       |     0|    0.00|        0|       0.00
    //   tcp://5678     |   300|   30.00|        1|       0.10
}

static void tableUsageExampleUpdate(bsl::ostream&     stream,
                                    bslma::Allocator* allocator)
{
    int                       numSnapshots = 11;
    mwcstm::StatContextUpdate update;
    mwcst::StatContext        context(
        mwcst::StatContextConfiguration("Network Interface")
            .isTable(true)
            .value("In", numSnapshots)
            .enableUpdateCollection(&update),
        allocator);

    bslma::ManagedPtr<mwcst::StatContext> client1 = context.addSubcontext(
        mwcst::StatContextConfiguration("tcp://1234"));
    bslma::ManagedPtr<mwcst::StatContext> client2 = context.addSubcontext(
        mwcst::StatContextConfiguration("tcp://5678"));

    int messageLength = 100;  // size of the message in bytes
    client1->adjustValue(0, messageLength);

    messageLength = 300;  // size of the message in bytes
    client2->adjustValue(0, messageLength);

    context.snapshot();

    // Create a second context whose values and configuration derive from
    // 'update', driven by 'context'.

    mwcst::StatContext updatedContext(
        mwcst::StatContextConfiguration(update, allocator));

    mwcst::StatContextTableInfoProvider tip;
    tip.setContext(&updatedContext);
    tip.setColumnGroup("");
    tip.addDefaultIdColumn("");
    tip.setColumnGroup("In");
    tip.addColumn("Bytes", 0, mwcst::StatUtil::value, 0);
    tip.addColumn("Bytes/s", 0, mwcst::StatUtil::rate, 10, 0);
    tip.addColumn("Messages", 0, mwcst::StatUtil::increments, 0);
    tip.addColumn("Messages/s", 0, mwcst::StatUtil::incrementRate, 10, 0);
    tip.update();

    if (verbose) {
        mwcu::TableUtil::printTable(stream, tip);
    }

    client1.clear();
    context.snapshot();
    updatedContext.snapshotFromUpdate(update);
    tip.update();
    if (verbose) {
        mwcu::TableUtil::printTable(stream, tip);
    }

    updatedContext.cleanup();
    tip.setContext(&updatedContext);
    tip.update();
    if (verbose) {
        mwcu::TableUtil::printTable(stream, tip);
    }
}

static void subcontextUsageExample(bsl::ostream&     stream,
                                   bslma::Allocator* allocator)
{
    // ------------------------------------------------------------------------
    // USAGE EXAMPLE WITH SUBCONTEXT
    //
    // Concerns:
    //   Test the usage example provided in the component header file.
    // ------------------------------------------------------------------------

    // This example demonstrates how to use the sub-context feature.  Suppose
    // we are keeping track of now much memory we use in our system, and we
    // want to keep track of the global allocator and of the allocator used
    // in our "Network Interface".

    // First we create a value, measuring a number of bytes in use.  The stats
    // we care about are min, max and average.  Note that for average it is
    // necessary to include a period, e.g. a number of snapshots.
    int numSnapshots = 11;

    // Then create a context for the whole system.
    mwcst::StatContext context(mwcst::StatContextConfiguration("Allocator")
                                   .value("Memory", numSnapshots),
                               allocator);

    // Now create a sub-context of the context, for the interface allocator.
    // Note that the context and any sub-contexts must share the same value
    // definitions. If you specify unrelated values to contexts and
    // sub-contexts, it will work but the printing of stats will show
    bslma::ManagedPtr<mwcst::StatContext> subContext = context.addSubcontext(
        mwcst::StatContextConfiguration("Interface Allocator")
            .value("Memory", numSnapshots));

    // Now lets record some data points.
    int memInUse = 50000;  // size of the allocator in bytes
    context.setValue(0, memInUse);

    memInUse = 1500;
    subContext->setValue(0, memInUse);

    // Finally lets snapshot and print the main context.
    context.snapshot();
    mwcst::StatContextTableInfoProvider tip;
    tip.setContext(&context);
    tip.setColumnGroup("");
    tip.addDefaultIdColumn("");
    tip.setColumnGroup("In");
    tip.addColumn("Bytes", 0, mwcst::StatUtil::value, 0);
    tip.addColumn("Avg", 0, mwcst::StatUtil::average, 10, 0);
    tip.addColumn("Min", 0, mwcst::StatUtil::absoluteMin);
    tip.addColumn("Max", 0, mwcst::StatUtil::absoluteMax);
    tip.update();
    if (verbose) {
        mwcu::TableUtil::printTable(stream, tip);
    }

    // This is what it prints:
    //
    //                      |            Memory
    //                      | Bytes |   Avg   | Min|  Max
    // ---------------------+-------+---------+----+-------
    // Allocator            | 50,000|  5000.00|   0| 50,000
    //   *direct*           | 50,000|  5000.00|   0| 50,000
    //   Interface Allocator|  1,500|   150.00|   0|  1,500
}

static void subcontextUsageExampleUpdate(bsl::ostream&     stream,
                                         bslma::Allocator* allocator)
{
    int                       numSnapshots = 11;
    mwcstm::StatContextUpdate update;
    mwcst::StatContext context(mwcst::StatContextConfiguration("Allocator")
                                   .value("Memory", numSnapshots)
                                   .enableUpdateCollection(&update),
                               allocator);

    bslma::ManagedPtr<mwcst::StatContext> subContext = context.addSubcontext(
        mwcst::StatContextConfiguration("Interface Allocator")
            .value("Memory", numSnapshots));

    int memInUse = 50000;  // size of the allocator in bytes
    context.setValue(0, memInUse);

    memInUse = 1500;
    subContext->setValue(0, memInUse);

    context.snapshot();

    // Create a second context whose values and configuration derive from
    // 'update', driven by 'context'.

    mwcst::StatContext updatedContext(
        mwcst::StatContextConfiguration(update, allocator));

    mwcst::StatContextTableInfoProvider tip;
    tip.setContext(&updatedContext);
    tip.setColumnGroup("");
    tip.addDefaultIdColumn("");
    tip.setColumnGroup("In");
    tip.addColumn("Bytes", 0, mwcst::StatUtil::value, 0);
    tip.addColumn("Avg", 0, mwcst::StatUtil::average, 10, 0);
    tip.addColumn("Min", 0, mwcst::StatUtil::absoluteMin);
    tip.addColumn("Max", 0, mwcst::StatUtil::absoluteMax);
    tip.update();
    if (verbose) {
        mwcu::TableUtil::printTable(stream, tip);
    }
}

static void valueLevelUsageExample(bsl::ostream&     stream,
                                   bslma::Allocator* allocator)
{
    // ------------------------------------------------------------------------
    // USAGE EXAMPLE WITH VALUE LEVEL
    //
    // Concerns:
    //   Demonstrate how to use the value level feature.
    // ------------------------------------------------------------------------

    // This example demonstrates how to use the value level feature, which is
    // used to aggregate the snapshots/samples. Here we capture 60 samples
    // of 1 second each, and then 5 samples of 1 minute each.  Each time the
    // 60 samples are done, they get aggregated and copied into the first
    // 1 minute sample.

    // First, create a context with 60 samples at level 0 and 5 samples at
    // level 1 ('valueLevel'):
    enum { e_IN = 0 };
    int                numSnapshots    = 60 + 1;
    int                numAggSnapshots = 5 + 1;
    mwcst::StatContext context(
        mwcst::StatContextConfiguration("Network Interface")
            .value("In", numSnapshots)
            .valueLevel(numAggSnapshots),
        allocator);

    // TIP to print the statistics in tabular format
    mwcst::StatContextTableInfoProvider tip;
    tip.setContext(&context);

    tip.setColumnGroup("");
    tip.addDefaultIdColumn("");

    tip.setColumnGroup("In");

    // Use the 'SnapshotLocation' function for the columns which use the minute
    // samples. First argument is the level number (e.g. 1), second argument
    // is the sample number.
    tip.addColumn("Bytes", e_IN, mwcst::StatUtil::value, 0);
    tip.addColumn("Bytes/s (last min)", e_IN, mwcst::StatUtil::rate, 60, 0);
    tip.addColumn("Bytes/s (5 min)",
                  e_IN,
                  mwcst::StatUtil::rate,
                  mwcst::StatValue::SnapshotLocation(1, 4),
                  mwcst::StatValue::SnapshotLocation(1, 0));

    tip.addColumn("Msg", e_IN, mwcst::StatUtil::increments, 0);
    tip.addColumn("Msg/s (last min)",
                  e_IN,
                  mwcst::StatUtil::incrementRate,
                  60,
                  0);
    tip.addColumn("Msg/s (5 min)",
                  e_IN,
                  mwcst::StatUtil::incrementRate,
                  mwcst::StatValue::SnapshotLocation(1, 4),
                  mwcst::StatValue::SnapshotLocation(1, 0));

    // Capture 5 minutes of data.
    int messageLength = 10;  // bytes

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 60; ++j) {
            // Snapshot 1 second
            context.adjustValue(e_IN, messageLength);
            context.snapshot();
        }

        // Print with the TIP after each minute
        tip.update();
        if (verbose) {
            stream << bsl::endl << "Minute number " << i;
            mwcu::TableUtil::printTable(stream, tip);
        }
    }
}

static void valueLevelUsageExampleUpdate(bsl::ostream&     stream,
                                         bslma::Allocator* allocator)
{
    mwcstm::StatContextUpdate update;
    enum { e_IN = 0 };
    int                numSnapshots    = 60 + 1;
    int                numAggSnapshots = 5 + 1;
    mwcst::StatContext context(
        mwcst::StatContextConfiguration("Network Interface")
            .value("In", numSnapshots)
            .valueLevel(numAggSnapshots)
            .enableUpdateCollection(&update),
        allocator);

    // Create a second context whose values and configuration derive from
    // 'update', driven by 'context'.

    mwcst::StatContext updatedContext(
        mwcst::StatContextConfiguration("Network Interface")
            .value("In", numSnapshots)
            .valueLevel(numAggSnapshots),
        allocator);

    mwcst::StatContextTableInfoProvider tip;
    tip.setContext(&updatedContext);

    tip.setColumnGroup("");
    tip.addDefaultIdColumn("");

    tip.setColumnGroup("In");

    // Use the 'SnapshotLocation' function for the columns which use the minute
    // samples. First argument is the level number (e.g. 1), second argument
    // is the sample number.
    tip.addColumn("Bytes", e_IN, mwcst::StatUtil::value, 0);
    tip.addColumn("Bytes/s (last min)", e_IN, mwcst::StatUtil::rate, 60, 0);
    tip.addColumn("Bytes/s (5 min)",
                  e_IN,
                  mwcst::StatUtil::rate,
                  mwcst::StatValue::SnapshotLocation(1, 4),
                  mwcst::StatValue::SnapshotLocation(1, 0));

    tip.addColumn("Msg", e_IN, mwcst::StatUtil::increments, 0);
    tip.addColumn("Msg/s (last min)",
                  e_IN,
                  mwcst::StatUtil::incrementRate,
                  60,
                  0);
    tip.addColumn("Msg/s (5 min)",
                  e_IN,
                  mwcst::StatUtil::incrementRate,
                  mwcst::StatValue::SnapshotLocation(1, 4),
                  mwcst::StatValue::SnapshotLocation(1, 0));

    // Capture 5 minutes of data.
    int messageLength = 10;  // bytes

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 60; ++j) {
            // Snapshot 1 second
            context.adjustValue(e_IN, messageLength);
            context.snapshot();
            updatedContext.snapshotFromUpdate(update);
        }

        // Print with the TIP after each minute
        tip.update();
        if (verbose) {
            stream << bsl::endl << "Minute number " << i;
            mwcu::TableUtil::printTable(stream, tip);
        }
    }
}

static void testUpdates(bslma::Allocator* /*allocator*/)
{
    // ------------------------------------------------------------------------
    // UPDATE TEST
    //
    // Concerns:
    //  That a 'StatValue' properly updates itself from a
    //  'mwcstm::StatValueUpdate', and that 'StatValueUtil' can properly
    //  initialize a 'StatValueUpdate' from a 'StatValue'.
    //
    // Plan:
    //  Create a variety of 'StatValue's, record a variable number of
    //  snapshots, then load/apply to and from a 'StatValueUpdate', and
    //  ensure the values are correct.
    //
    // Testing:
    //  setFromUpdate()
    //  StatValueUtil::loadUpdate()
    //  StatValueUtil::loadFullUpdate()
    //
    // ------------------------------------------------------------------------

    PV("Verify updates are loaded and can be applied.");

    typedef mwcstm::StatValueFields Fields;

    mwcstm::StatContextUpdate u1, u2;
    StatContext               c1(StatContextConfiguration("context1")
                       .value("c", StatValue::DMCST_CONTINUOUS, 3)
                       .valueLevel(2)
                       .valueLevel(1)
                       .value("d", StatValue::DMCST_DISCRETE, 2)
                       .valueLevel(1)
                       .enableUpdateCollection(&u1));

    ASSERT(checkSnapshot(direct(c1, 0), 0, 0, "0 0 0 0 0"));
    ASSERT(checkSnapshot(direct(c1, 1), 0, 0, "++ -- 0 0"));

    c1.adjustValue(0, 5);
    c1.adjustValue(0, 4);
    c1.reportValue(1, 5);
    c1.reportValue(1, 4);
    c1.snapshot();

    mwcstm::StatContextUpdate fullUpdate;
    c1.loadFullUpdate(&fullUpdate);
    ASSERT_EQUALS(u1, fullUpdate);

    ASSERT(2 == u1.directValues().size());
    ASSERT(hasField(u1, 0, Fields::DMCSTM_ABSOLUTE_MIN));
    ASSERT(hasField(u1, 0, Fields::DMCSTM_ABSOLUTE_MAX));
    ASSERT(hasField(u1, 0, Fields::DMCSTM_MIN));
    ASSERT(hasField(u1, 0, Fields::DMCSTM_MAX));
    ASSERT(hasField(u1, 0, Fields::DMCSTM_VALUE));
    ASSERT(hasField(u1, 0, Fields::DMCSTM_INCREMENTS));
    ASSERT(hasField(u1, 0, Fields::DMCSTM_DECREMENTS));
    ASSERT(!hasField(u1, 0, Fields::DMCSTM_EVENTS));
    ASSERT(!hasField(u1, 0, Fields::DMCSTM_SUM));
    ASSERT_EQUALS(0, field(u1, 0, Fields::DMCSTM_ABSOLUTE_MIN));
    ASSERT_EQUALS(9, field(u1, 0, Fields::DMCSTM_ABSOLUTE_MAX));
    ASSERT_EQUALS(0, field(u1, 0, Fields::DMCSTM_MIN));
    ASSERT_EQUALS(9, field(u1, 0, Fields::DMCSTM_MAX));
    ASSERT_EQUALS(9, field(u1, 0, Fields::DMCSTM_VALUE));
    ASSERT_EQUALS(2, field(u1, 0, Fields::DMCSTM_INCREMENTS));
    ASSERT_EQUALS(0, field(u1, 0, Fields::DMCSTM_DECREMENTS));

    ASSERT(hasField(u1, 1, Fields::DMCSTM_ABSOLUTE_MIN));
    ASSERT(hasField(u1, 1, Fields::DMCSTM_ABSOLUTE_MAX));
    ASSERT(hasField(u1, 1, Fields::DMCSTM_MIN));
    ASSERT(hasField(u1, 1, Fields::DMCSTM_MAX));
    ASSERT(!hasField(u1, 1, Fields::DMCSTM_VALUE));
    ASSERT(!hasField(u1, 1, Fields::DMCSTM_INCREMENTS));
    ASSERT(!hasField(u1, 1, Fields::DMCSTM_DECREMENTS));
    ASSERT(hasField(u1, 1, Fields::DMCSTM_EVENTS));
    ASSERT(hasField(u1, 1, Fields::DMCSTM_SUM));
    ASSERT_EQUALS(4, field(u1, 1, Fields::DMCSTM_ABSOLUTE_MIN));
    ASSERT_EQUALS(5, field(u1, 1, Fields::DMCSTM_ABSOLUTE_MAX));
    ASSERT_EQUALS(4, field(u1, 1, Fields::DMCSTM_MIN));
    ASSERT_EQUALS(5, field(u1, 1, Fields::DMCSTM_MAX));
    ASSERT_EQUALS(2, field(u1, 1, Fields::DMCSTM_EVENTS));
    ASSERT_EQUALS(9, field(u1, 1, Fields::DMCSTM_SUM));

    StatContext c2(StatContextConfiguration(u1).enableUpdateCollection(&u2));
    ASSERT_EQUALS(u1, u2);
    ASSERT(checkSnapshot(direct(c2, 0), 0, 0, "9 0 9 2 0"));
    ASSERT(checkSnapshot(direct(c2, 0), 0, 1, "0 0 0 0 0"));
    ASSERT(checkSnapshot(direct(c2, 0), 1, 0, "0 0 0 0 0"));

    ASSERT(checkSnapshot(direct(c2, 1), 0, 0, "4 5 2 9"));
    ASSERT(checkSnapshot(direct(c2, 1), 0, 1, "++ -- 0 0"));
    ASSERT(checkSnapshot(direct(c2, 1), 1, 0, "++ -- 0 0"));

    // Adjust the value and take another snapshot
    c2.adjustValue(0, 1);
    c2.adjustValue(0, -2);
    c2.reportValue(1, 10);
    c2.snapshot();
    ASSERT(u2.configuration().isNull());

    ASSERT(!hasField(u2, 0, Fields::DMCSTM_ABSOLUTE_MIN));
    ASSERT(!hasField(u2, 0, Fields::DMCSTM_ABSOLUTE_MAX));
    ASSERT(hasField(u2, 0, Fields::DMCSTM_MIN));
    ASSERT(hasField(u2, 0, Fields::DMCSTM_MAX));
    ASSERT(hasField(u2, 0, Fields::DMCSTM_VALUE));
    ASSERT(hasField(u2, 0, Fields::DMCSTM_INCREMENTS));
    ASSERT(hasField(u2, 0, Fields::DMCSTM_DECREMENTS));
    ASSERT(!hasField(u2, 0, Fields::DMCSTM_EVENTS));
    ASSERT(!hasField(u2, 0, Fields::DMCSTM_SUM));
    ASSERT_EQUALS(8, field(u2, 0, Fields::DMCSTM_MIN));
    ASSERT_EQUALS(10, field(u2, 0, Fields::DMCSTM_MAX));
    ASSERT_EQUALS(8, field(u2, 0, Fields::DMCSTM_VALUE));
    ASSERT_EQUALS(3, field(u2, 0, Fields::DMCSTM_INCREMENTS));
    ASSERT_EQUALS(1, field(u2, 0, Fields::DMCSTM_DECREMENTS));

    ASSERT(!hasField(u2, 1, Fields::DMCSTM_ABSOLUTE_MIN));
    ASSERT(!hasField(u2, 1, Fields::DMCSTM_ABSOLUTE_MAX));
    ASSERT(hasField(u2, 1, Fields::DMCSTM_MIN));
    ASSERT(hasField(u2, 1, Fields::DMCSTM_MAX));
    ASSERT(!hasField(u2, 1, Fields::DMCSTM_VALUE));
    ASSERT(!hasField(u2, 1, Fields::DMCSTM_INCREMENTS));
    ASSERT(!hasField(u2, 1, Fields::DMCSTM_DECREMENTS));
    ASSERT(hasField(u2, 1, Fields::DMCSTM_EVENTS));
    ASSERT(hasField(u2, 1, Fields::DMCSTM_SUM));
    ASSERT_EQUALS(10, field(u2, 1, Fields::DMCSTM_MIN));
    ASSERT_EQUALS(10, field(u2, 1, Fields::DMCSTM_MAX));
    ASSERT_EQUALS(3, field(u2, 1, Fields::DMCSTM_EVENTS));
    ASSERT_EQUALS(19, field(u2, 1, Fields::DMCSTM_SUM));

    c1.snapshotFromUpdate(u2);
    ASSERT_EQUALS(u1, u2);

    ASSERT(checkSnapshot(direct(c1, 0), 0, 0, "8 8 10 3 1"));
    ASSERT(checkSnapshot(direct(c1, 0), 0, 1, "9 0 9 2 0"));
    ASSERT(checkSnapshot(direct(c1, 0), 0, 2, "0 0 0 0 0"));
    ASSERT(checkSnapshot(direct(c1, 0), 1, 0, "0 0 0 0 0"));

    ASSERT(checkSnapshot(direct(c1, 1), 0, 0, "10 10 3 19"));
    ASSERT(checkSnapshot(direct(c1, 1), 0, 1, "4 5 2 9"));
    ASSERT(checkSnapshot(direct(c1, 1), 1, 0, "4 10 3 19"));

    // Take a third snapshot.  This should update level 2
    c1.adjustValue(0, 3);

    c1.snapshot();

    ASSERT(!hasField(u1, 1, Fields::DMCSTM_ABSOLUTE_MIN));
    ASSERT(!hasField(u1, 1, Fields::DMCSTM_ABSOLUTE_MAX));
    ASSERT(hasField(u1, 1, Fields::DMCSTM_MIN));
    ASSERT(hasField(u1, 1, Fields::DMCSTM_MAX));
    ASSERT(!hasField(u1, 1, Fields::DMCSTM_VALUE));
    ASSERT(!hasField(u1, 1, Fields::DMCSTM_INCREMENTS));
    ASSERT(!hasField(u1, 1, Fields::DMCSTM_DECREMENTS));
    ASSERT(!hasField(u1, 1, Fields::DMCSTM_EVENTS));
    ASSERT(!hasField(u1, 1, Fields::DMCSTM_SUM));
    ASSERT_EQUALS(MAX_INT, field(u1, 1, Fields::DMCSTM_MIN));
    ASSERT_EQUALS(MIN_INT, field(u1, 1, Fields::DMCSTM_MAX));

    c2.snapshotFromUpdate(u1);
    ASSERT_EQUALS(u1, u2);

    ASSERT(checkSnapshot(direct(c2, 0), 0, 0, "11 8 11 4 1"));
    ASSERT(checkSnapshot(direct(c2, 0), 0, 1, "8 8 10 3 1"));
    ASSERT(checkSnapshot(direct(c2, 0), 0, 2, "9 0 9 2 0"));
    ASSERT(checkSnapshot(direct(c2, 0), 1, 0, "11 0 11 4 1"));
    ASSERT(checkSnapshot(direct(c2, 0), 1, 1, "0 0 0 0 0"));
    ASSERT(checkSnapshot(direct(c2, 0), 2, 0, "0 0 0 0 0"));

    ASSERT(checkSnapshot(direct(c2, 1), 0, 0, "++ -- 3 19"));
    ASSERT(checkSnapshot(direct(c2, 1), 0, 1, "10 10 3 19"));
    ASSERT(checkSnapshot(direct(c2, 1), 1, 0, "4 10 3 19"));
}

static void testUpdatesWithUsageExample(bslma::Allocator* allocator)
{
    PV("Test usage examples using updates");

    mwcu::MemOutStream nstream;
    mwcu::MemOutStream ustream;

    PV("a. Test basic usage example");
    usageExample(nstream, allocator);
    usageExampleUpdate(ustream, allocator);

    ASSERT_EQUALS(nstream.str(), ustream.str());

    nstream.reset();
    ustream.reset();

    PV("b. Test table usage example");
    tableUsageExample(nstream, allocator);
    tableUsageExampleUpdate(ustream, allocator);

    ASSERT_EQUALS(nstream.str(), ustream.str());

    nstream.reset();
    ustream.reset();

    PV("c. Test subcontext usage example");
    subcontextUsageExample(nstream, allocator);
    subcontextUsageExampleUpdate(ustream, allocator);

    ASSERT_EQUALS(nstream.str(), ustream.str());

    nstream.reset();
    ustream.reset();

    PV("d. Test value level example");
    valueLevelUsageExample(nstream, allocator);
    valueLevelUsageExampleUpdate(ustream, allocator);

    ASSERT_EQUALS(nstream.str(), ustream.str());
}

static void testDatum(bslma::Allocator* allocator)
{
    mwcst::StatContextConfiguration config("test");
    mwcst::StatContext              context(config, allocator);

    bslma::ManagedPtr<bdld::ManagedDatum> datum = context.datum();

    *datum = bdld::ManagedDatum(bdld::Datum::createInteger(10), allocator);

    datum.clear();

    datum = context.datum();

    ASSERT(datum->datum().isInteger());
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    int test            = argc > 1 ? atoi(argv[1]) : 0;
    verbose             = argc > 2;
    veryVerbose         = argc > 3;
    veryVeryVerbose     = argc > 4;
    veryVeryVeryVerbose = argc > 5;
    // Prevent potential compiler unused warning
    (void)verbose;
    (void)veryVerbose;
    (void)veryVeryVerbose;
    (void)veryVeryVeryVerbose;

    // Initialize BALL
    INIT_BALL_LOGGING_VERBOSITY(verbose, veryVerbose);

    // Initialize default and global memory allocators
    bslma::TestAllocator ga("global", veryVeryVeryVerbose);
    ga.setNoAbort(true);
    bslma::Default::setGlobalAllocator(&ga);

    bslma::TestAllocator da("default", veryVeryVeryVerbose);
    da.setNoAbort(true);
    bslma::Default::setDefaultAllocator(&da);

    bslma::TestAllocator ta("test", veryVeryVeryVerbose);
    ta.setNoAbort(true);

    bslma::TestAllocatorMonitor gam(&ga), dam(&da);

    bsls::AssertFailureHandlerGuard g(bsls::AssertTest::failTestDriver);

    cout << "TEST " << __FILE__ << " CASE " << test << endl;

    switch (test) {
    case 0:  // Zero is always the leading case.
    case 7: {
        // --------------------------------------------------------------------
        // TEST DATUM
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "TEST DATUM" << endl << "==========" << endl;
        testDatum(&ta);
    } break;

    case 6: {
        // --------------------------------------------------------------------
        // UPDATE USAGE EXAMPLE TEST
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl
                 << "USAGE EXAMPLES WITH UPDATES" << endl
                 << "===========================" << endl;
        testUpdatesWithUsageExample(&ta);
    } break;

    case 5: {
        // --------------------------------------------------------------------
        // TEST UPDATES
        // --------------------------------------------------------------------

        if (verbose)
            cout << endl << "TEST UPDATES" << endl << "============" << endl;
        testUpdates(&ta);
    } break;

    case 4: {
        // --------------------------------------------------------------------
        // USAGE EXAMPLE WITH VALUE LEVEL
        // --------------------------------------------------------------------
        if (verbose)
            cout << endl
                 << "USAGE EXAMPLE WITH VALUE LEVEL" << endl
                 << "==============================" << endl;
        valueLevelUsageExample(cout, &ta);
    } break;

    case 3: {
        // --------------------------------------------------------------------
        // USAGE EXAMPLE WITH SUBCONTEXT
        // --------------------------------------------------------------------
        if (verbose)
            cout << endl
                 << "USAGE EXAMPLE WITH SUBCONTEXT" << endl
                 << "=============================" << endl;
        subcontextUsageExample(cout, &ta);
    } break;

    case 2: {
        // --------------------------------------------------------------------
        // USAGE EXAMPLE WITH TABLE
        // --------------------------------------------------------------------
        if (verbose)
            cout << endl
                 << "USAGE EXAMPLE WITH TABLES" << endl
                 << "=========================" << endl;
        tableUsageExample(cout, &ta);
    } break;

    case 1: {
        // --------------------------------------------------------------------
        // USAGE EXAMPLE
        // --------------------------------------------------------------------
        if (verbose)
            cout << endl << "USAGE EXAMPLE" << endl << "=============" << endl;
        usageExample(cout, &ta);
    } break;

    default:
        cerr << "WARNING: CASE `" << test << "' NOT FOUND." << endl;
        testStatus = -1;
        break;
    }

    // Ensure no memory was allocated from the default or global allocator
    ASSERT_EQUALS(gam.isTotalSame(), true);
    ASSERT_EQUALS(dam.isTotalSame(), true);

    // The standard return values are 0 on success, a positive value
    // to indicate the number of assertion failures, or a negative
    // value to indicate that the test case was not found.  Special value 254
    // is used to skip a test, for example for Jenkins.
    mwcu::TestUtil::printTestStatus(testStatus, verbose);
    return testStatus;
}
