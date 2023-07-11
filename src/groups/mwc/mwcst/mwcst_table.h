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

// mwcst_table.h                                                      -*-C++-*-
#ifndef INCLUDED_MWCST_TABLE
#define INCLUDED_MWCST_TABLE

//@PURPOSE: Provide a simple table of values derived from a table stat context.
//
//@CLASSES:
// mwcst::Table : table of values backed by a 'mwcst::StatContext'.
//
//@SEE_ALSO: mwcst_statcontext
//           mwcst_tableschema
//           mwcst_tablerecord
//
//@DESCRIPTION: This component defines a mechanism, 'mwcst::Table', which is a
// table of values derived from a table 'mwcst::StatContext'.
//
// A 'mwcst::Table' must be associated with a 'mwcst::StatContext' after
// creation; the stat context will provide the values of the table.  The table
// itself has two parts: a "schema" e.g. a set of colums, and a set of records
// for backed by the stat context.  The schema is implemented by a
// 'mwcst::TableSchema' and the records are defined by a 'mwcst::TableRecords'.
//
// The application defines the schema of a table by adding named columns that
// are defined using functions (from 'mwcst::StatUtil') and their arguments.
// For example, the application can define a column named "total" as the
// value of the first snapshot of a particular variable/value in the stat
// context.
//
// The records of the table are backed by the stat context, so there isn't
// much to be done here.  However the application can set a filtering function
// to eliminate some records (for example to filter out "*direct*" values),
// as well as a sorting function to display records in a particular order.
//
/// Usage Example
///-------------
// Suppose we are creating an application that needs to keep track of how
// much IO traffic it performs over an interface (TCP or whatever), and prints
// it on a regular basis.  The application creates a 'mwcst::StatContext' with
// two 'mwcst::Value': "IN" representing the input traffic, and "OUT"
// representing the output traffic.  The stat context can be initialized using
// a 'mwcst::StatContextConfiguration' created by the following function:
//..
//  mwcst::StatContextConfiguration
//  createStatContextConfiguration(int               numSamples,
//                                 bslma::Allocator *allocator)
//  {
//       mwcst::StatContextConfiguration config("Total", allocator);
//       config.isTable(true);
//       config.value("IN", numSamples)
//       config.value("OUT", numSamples)
//       return config;
//  }
//..
// To print the stat context, the application first creates a 'mwcst::Table'
// and initializes it using the following function:
//..
//  void initTable(mwcst::Table       *table,
//                 mwcst::StatContext *statContext,
//                 int                 numSamples)
//  {
//      typedef mwcst::StatUtil SU;
//      typedef mwcst::StatValue SV;
//
//      int inValueIndex = 0;
//      int outValueIndex = 1;
//
//      mwcst::TableSchema& schema = table->schema();
//      schema.addDefaultIdColumn("Id");
//
//      // In
//      schema.addColumn("IN Bytes", inValueIndex, SU::value, 0);
//      schema.addColumn("IN Bytes Delta",
//                       inValueIndex,
//                       SU::valueDifference,
//                       SV::SnapshotLocation(0, 0),
//                       SV::SnapshotLocation(0, numSamples));
//      schema.addColumn("IN Bytes Rate",
//                       inValueIndex,
//                       SU::ratePerSecond,
//                       SV::SnapshotLocation(0, numSamples),
//                       SV::SnapshotLocation(0, 0));
//
//      schema.addColumn("IN Items", inValueIndex, SU::increments, 0);
//      schema.addColumn("IN Items Delta",
//                       inValueIndex,
//                       SU::incrementsDifference,
//                       SV::SnapshotLocation(0, 0),
//                       SV::SnapshotLocation(0, numSamples));
//      schema.addColumn("IN Items Rate",
//                       inValueIndex,
//                       SU::incrementsPerSecond,
//                       SV::SnapshotLocation(0, numSamples),
//                       SV::SnapshotLocation(0, 0));
//
//      // Out
//      schema.addColumn("OUT Bytes", outValueIndex, SU::value, 0);
//      schema.addColumn("OUT Bytes Delta",
//                       outValueIndex,
//                       SU::valueDifference,
//                       SV::SnapshotLocation(0, 0),
//                       SV::SnapshotLocation(0, numSamples));
//      schema.addColumn("OUT Bytes Rate",
//                       outValueIndex,
//                       SU::ratePerSecond,
//                       SV::SnapshotLocation(0, numSamples),
//                       SV::SnapshotLocation(0, 0));
//
//      schema.addColumn("OUT Items", outValueIndex, SU::increments, 0);
//      schema.addColumn("OUT Items Delta",
//                       outValueIndex,
//                       SU::incrementsDifference,
//                       SV::SnapshotLocation(0, 0),
//                       SV::SnapshotLocation(0, numSamples));
//      schema.addColumn("OUT Items Rate",
//                       outValueIndex,
//                       SU::incrementsPerSecond,
//                       SV::SnapshotLocation(0, numSamples),
//                       SV::SnapshotLocation(0, 0));
//
//      // Associate the table record to the stat context.
//      mwcst::TableRecords& records = table->records();
//      records.setContext(statContext);
//
//      // Note shown here: we could add a filtering and a sorting function
//      // to 'records' if needed.
//  }
//..
// This function initializes a generic table for capturing IO statistics.
// The table columns refer to bytes and "items", which could be packets or
// messages.  The table is associated with the stat context.  Note that the
// argument 'numSamples' is the number of samples in the stat context.
//
// Now it is time to print this table, by creating a
// 'mwcu::BasicTableInfoProvider' associated with the table (pass the table
// in the constructor, and initializing it using the following function:
//..
//  void initTIP(mwcu::BasicTableInfoProvider *tip,
//               int                           numSamples)
//  {
//      // ID
//      tip->setColumnGroup("");
//      tip->addColumn("Id", "").justifyLeft();
//
//      // In
//      tip->setColumnGroup("IN");
//      tip->addColumn("IN Bytes", "Bytes").zeroString(" ");
//      tip->addColumn("IN Bytes Delta", "Delta").zeroString(" ");
//      tip->addColumn("IN Bytes Rate", "Bytes/s").zeroString(" ");
//      tip->addColumn("IN Items", "Pkt").zeroString(" ");
//      tip->addColumn("IN Items Delta", "Delta").zeroString(" ");
//      tip->addColumn("IN Items Rate", "Pkt/s").zeroString(" ");
//
//      // Out
//      tip->setColumnGroup("OUT");
//      tip->addColumn("OUT Bytes", "Bytes").zeroString(" ");
//      tip->addColumn("OUT Bytes Delta", "Delta").zeroString(" ");
//      tip->addColumn("OUT Bytes Rate", "Bytes/s").zeroString(" ");
//      tip->addColumn("OUT Items", "Pkt").zeroString(" ");
//      tip->addColumn("OUT Items Delta", "Delta").zeroString(" ");
//      tip->addColumn("OUT Items Rate", "Pkt/s").zeroString(" ");
//  }
//..
// This function associates the columns of the table with the columns of the
// table info provider.  It also sets up column groups and formatting options
// like displaying blanks instead of zeros.  Refer to the documentation
// in 'mwcu::TableUtil' for details.  The table that will be printed looks
// like this (only showing IN; OUT is identical):
//..
//                |                    IN                     |
//                |  Bytes|  Delta| Bytes/s| Pkt| Delta| Pkt/s|
//   -------------+-------+-------+--------+----+------+------+
//   Total        | 28,320| 28,320|  480.00| 120|   120|  2.00|
//     foo.tsk:123| 28,320| 28,320|  484.00| 120|   120|  2.00|
//..

#include <mwcst_tablerecords.h>
#include <mwcst_tableschema.h>
#include <mwcst_utable.h>

namespace BloombergLP {
namespace mwcst {

// ===========
// class Table
// ===========

/// Table of values obtained from a `mwcst::StatContext`.
class Table : public mwcu::Table {
  private:
    // DATA
    TableSchema d_schema;

    TableRecords d_records;

    // NOT IMPLEMENTED
    Table(const Table&);
    Table& operator=(const Table&);

  public:
    // CREATORS
    Table(bslma::Allocator* allocator = 0);

    // MANIPULATORS
    TableSchema& schema();

    /// Return a modifiable reference to the table's schema and records,
    /// respectively.
    TableRecords& records();

    // ACCESSORS

    /// Return the number of columns.
    int numColumns() const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of rows.
    int numRows() const BSLS_KEYWORD_OVERRIDE;

    /// Return the name of the specified `column` index.
    bslstl::StringRef columnName(int column) const BSLS_KEYWORD_OVERRIDE;

    /// Load the specified `value` with the value located at the specified
    /// `row` and `column` indices.
    void
    value(mwct::Value* value, int row, int column) const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
