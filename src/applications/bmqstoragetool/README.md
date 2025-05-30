BMQStorageTool
==============

BMQStorageTool is a command-line tool for analyzing of BlazingMQ Broker storage
files.  Using a set of different filters, it allows to search records in:
- `journal` file (*.bmq_journal).
- `cluster state ledger` (CSL) file (*.bmq_csl).

The output results can be returned in either short or detailed form.

As an input, either a `journal` file (*.bmq_journal) or `cluster state ledger` 
(CSL) file (*.bmq_csl) is **always** required.

In case of `journal` file search:
 - to dump payload, `data` file (*.bmq_data) is required.
 - to filter by queue Uri, cluster state ledger (CSL) file (*.bmq_csl) is required.
 
 In case of `cluster state ledger` (CSL) file search, only CSL file (*.bmq_csl) must be passed.

The tool can be found under your `CMAKE` build directory after making 
the project. From the command-line, there are a few options you can use when
invoking the tool.

```bash
Usage:   bmqstoragetool [-r|record-type <record type>]*
                        [--csl-record-type <csl record type>]*
                        [--journal-path <journal path>]
                        [--journal-file <journal file>]
                        [--data-file <data file>]
                        [--csl-file <csl file>]
                        [--csl-from-begin]
                        [--print-mode <print mode>]
                        [--guid <guid>]*
                        [--seqnum <seqnum>]*
                        [--offset <offset>]*
                        [--queue-name <queue name>]*
                        [--queue-key <queue key>]*
                        [--timestamp-gt <timestamp greater than>]
                        [--timestamp-lt <timestamp less than>]
                        [--seqnum-gt <composite sequence number greater than>]
                        [--seqnum-lt <composite sequence number less than>]
                        [--offset-gt <offset greater than>]
                        [--offset-lt <offset less than>]
                        [--outstanding]
                        [--confirmed]
                        [--partially-confirmed]
                        [--details]
                        [--dump-payload]
                        [--dump-limit <dump limit>]
                        [--summary]
                        [--min-records-per-queue <threshold>]
                        [--summary-queues-limit <queues limit>]                        
                        [-h|help]
Where:
  -r | --record-type          <record type>
          record type to search {<message>|queue-op|journal-op} (default: message)
       --csl-record-type      <csl record type>
          CSL record type to search {<snapshot>|update|commit|ack} (default: all record types)
       --journal-path         <pattern>
          '*'-ended file path pattern, where the tool will try to find journal
          and data files
       --journal-file         <journal file>
          path to a .bmq_journal file
       --data-file            <data file>
          path to a .bmq_data file
       --csl-file             <csl file>
          path to a .bmq_csl file
       --csl-from-begin
          force to iterate CSL file from the beginning. By default: iterate from the latest snapshot
       --print-mode           <print mode>
          can be one of the following {<human>|json-pretty|json-line} (default: human)
       --guid                 <guid>
          message guid
       --seqnum               <seqnum>
          message composite sequence number
       --offset               <offset>
          message offset
       --queue-name           <queue name>
          message queue name
       --queue-key            <queue key>
          message queue key
       --timestamp-gt         <timestamp greater than>
          lower timestamp bound
       --timestamp-lt         <timestamp less than>
          higher timestamp bound
       --seqnum-gt            <composite sequence number greater than>
          lower composite sequence number bound, defined in form <leaseId-sequenceNumber>, e.g. 123-456
       --seqnum-lt            <composite sequence number less than>
          higher composite sequence number bound, defined in form <leaseId-sequenceNumber>, e.g. 123-456
       --offset-gt            <offset greater than>
          lower offset bound
       --offset-lt            <offset less than>
          higher offset bound
       --outstanding
          show only outstanding (not deleted) messages
       --confirmed
          show only messages, confirmed by all the appId's
       --partially-confirmed
          show only messages, confirmed by some of the appId's
       --details
          specify if you need message details
       --dump-payload
          specify if you need message payload
       --dump-limit           <dump limit>
          limit of payload output (default: 1024)
       --min-records-per-queue
         min number of records per queue for detailed info to be displayed
       --summary
          summary of all matching messages (number of outstanding messages and
          other statistics)
       --summary-queues-limit   <queues limit>
          limit of queues to display in CSL file summary (default: 50)
  -h | --help
          print usage
```

Scenarios of BMQStorageTool usage for journal file
==================================================

Output summary for journal file
-------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --summary
```

Search and otput all message GUIDs in journal file
--------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path>
```

Search and otput all queueOp/journalOp records or all records in journal file
-----------------------------------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --record-type=queue-op
./bmqstoragetool.tsk --journal-file=<path> --record-type=journal-op
./bmqstoragetool.tsk --journal-file=<path> --record-type=journal-op --record-type=queue-op --record-type=message
```

Search and otput all messages details in journal file
-----------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --details
```

Search and otput all outstanding/confirmed/partially-confirmed message GUIDs in journal file
--------------------------------------------------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --outstanding
./bmqstoragetool.tsk --journal-file=<path> --confirmed 
./bmqstoragetool.tsk --journal-file=<path> --partially-confirmed 
```

Search all message GUIDs with payload dump in journal file
----------------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<journal-path> --data-file=<data-path> --dump-payload
./bmqstoragetool.tsk --journal-path=<path.*> --dump-payload
./bmqstoragetool.tsk --journal-path=<path.*> --dump-payload --dump-limit=64
```

Scenarios of BMQStorageTool usage for CSL file
==============================================

Output summary for CSL file
---------------------------
Example:
```bash
./bmqstoragetool.tsk --csl-file=<path> --summary
./bmqstoragetool.tsk --csl-file=<path> --summary --summary-queues-limit=100
```

Output records from the beginning of CSL file 
---------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --csl-file=<path> --csl-from-begin
```
NOTE: by default search is done from the latest snapshot.

Search and otput only desired record types in CSL file
------------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --csl-file=<path> --csl-record-type=snapshot --csl-record-type=update
```
NOTE: `snapshot`, `update`, `commit` and `ack ` are supported. Without this option all record types are selected.

Search and otput records details in CSL file
--------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --csl-file=<path> --details
```

Applying search filters to above scenarios
==========================================

Filter messages with corresponding GUIDs
----------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --guid=<guid_1> --guid=<guid_N>
```
NOTE: no other filters are allowed with this one. Not suitable for CSL file search.

Filter messages with corresponding composite sequence numbers
-------------------------------------------------------------

Composite sequence numbers are defined in form `primaryLeaseId-sequenceNumber` for journal file or `electorTerm-sequenceNumber` for CSL file.

Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --seqnum=<leaseId1-sequenceNumber_1> --seqnum=<leaseId_N-sequenceNumber_N>
./bmqstoragetool.tsk --csl-file=<path> --seqnum=<electorTerm1-sequenceNumber_1> --seqnum=<electorTerm_N-sequenceNumber_N>
```
NOTE: no other filters are allowed with this one

Filter messages with corresponding record offsets
-------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --offset=<offset_1> --offset=<offset_N>
./bmqstoragetool.tsk --csl-file=<path> --offset=<offset_1> --offset=<offset_N>
```
NOTE: no other filters are allowed with this one

Filter messages with corresponding composite sequence numbers (defined in form <primaryLeaseId-sequenceNumber>)
---------------------------------------------------------------------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --seqnum=<leaseId-sequenceNumber_1> --seqnum=<leaseId-sequenceNumber_N>
```
NOTE: no other filters are allowed with this one

Filter messages with corresponding record offsets
-------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --offset=<offset_1> --offset=<offset_N>
```
NOTE: no other filters are allowed with this one

Filter messages within time range
---------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --timestamp-lt=<stamp>
./bmqstoragetool.tsk --journal-file=<path> --timestamp-gt=<stamp>
./bmqstoragetool.tsk --journal-file=<path> --timestamp-lt=<stamp1> --timestamp-gt=<stamp2>
./bmqstoragetool.tsk --csl-file=<path> --timestamp-lt=<stamp1> --timestamp-gt=<stamp2>
```

Filter messages within composite sequence numbers (primaryLeaseId/electorTerm, sequenceNumber) range
----------------------------------------------------------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --seqnum-lt=<leaseId-sequenceNumber>
./bmqstoragetool.tsk --journal-file=<path> --seqnum-gt=<leaseId-sequenceNumber>
./bmqstoragetool.tsk --journal-file=<path> --seqnum-lt=<leaseId1-sequenceNumber1> --seqnum-gt=<leaseId2-sequenceNumber2>
./bmqstoragetool.tsk --csl-file=<path> --seqnum-lt=<electorTerm1-sequenceNumber1> --seqnum-gt=<electorTerm2-sequenceNumber2>
```

Filter messages within record offsets range
-------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --offset-lt=<offset>
./bmqstoragetool.tsk --journal-file=<path> --offset-gt=<offset>
./bmqstoragetool.tsk --journal-file=<path> --offset-lt=<offset1> --offset-gt=<offset2>
./bmqstoragetool.tsk --csl-file=<path> --offset-lt=<offset1> --offset-gt=<offset2>
```

Filter messages within composite sequence numbers (primaryLeaseId, sequenceNumber) range
----------------------------------------------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --seqnum-lt=<leaseId-sequenceNumber>
./bmqstoragetool.tsk --journal-file=<path> --seqnum-gt=<leaseId-sequenceNumber>
./bmqstoragetool.tsk --journal-file=<path> --seqnum-lt=<leaseId1-sequenceNumber1> --seqnum-gt=<leaseId2-sequenceNumber2>
```

Filter messages within record offsets range
-------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --offset-lt=<offset>
./bmqstoragetool.tsk --journal-file=<path> --offset-gt=<offset>
./bmqstoragetool.tsk --journal-file=<path> --offset-lt=<offset1> --offset-gt=<offset2>
```

Filter messages by queue key
----------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --queue-key=<key_1> --queue-key=<key_N>
./bmqstoragetool.tsk --csl-file=<path> --queue-key=<key_1> --queue-key=<key_N>
```

Filter messages by queue Uri
----------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<journal_path> --csl-file=<csl_path> --queue-name=<queue_uri_1> --queue-name=<queue_uri_N>
./bmqstoragetool.tsk --csl-file=<csl_path> --queue-name=<queue_uri_1> --queue-name=<queue_uri_N>
```
NOTE: CSL file is required

Output search results in machine readable (JSON) format for all above scenarios
===============================================================================

Output in JSON pretty format 
----------------------------
Example:
```bash
./bmqstoragetool.tsk --print-mode=json-pretty
```

Output in JSON line format 
--------------------------
Example:
```bash
./bmqstoragetool.tsk --print-mode=json-line
```

Display number of records per type (e.g. Message, Confirm, Delete, etc.) per queue.
The number of Confirm records are displayed per AppId if there are more than 1 AppId.
The information is displayed for the queues with a total number of records greater or
equal to the value of `--min-records-per-queue` param.
By default this feature is disabled.
-------------------------------------------------------------------------------------
Example:
```bash
./bmqstoragetool.tsk --journal-file=<path> --min-records-per-queue=<limit>
```
