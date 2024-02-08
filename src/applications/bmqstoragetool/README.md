BMQStorageTool
==============

BMQStorageTool is a command-line tool for analyzing of BlazingMQ Broker storage
files. It allows to search messages in `journal` file with set of different 
filters and output found message GUIDs or message detail information. 
As input it expects `journal` file (*.bmq_journal)
and `data` file (*.bmq_data) if payload dump is required. Cluster state
ledger (CSL) file (*.bmq_csl) is required to filter by queue Uri.

The tool can be found under your `CMAKE` build directory after making 
the project. From the command-line, there are a few options you can use when
invoking the tool.

```bash
Usage:   bmqstoragetool.tsk [--journal-path <journal path>]
                            [--journal-file <journal file>]
                            [--data-file <data file>]
                            [--csl-file <csl file>]
                            [--guid <guid>]*
                            [--queue-name <queue name>]*
                            [--queue-key <queue key>]*
                            [--timestamp-gt <timestamp greater than>]
                            [--timestamp-lt <timestamp less than>]
                            [--outstanding]
                            [--confirmed]
                            [--partially-confirmed]
                            [--details]
                            [--dump-payload]
                            [--dump-limit <dump limit>]
                            [--summary]
                            [-h|help]
Where:
       --journal-path <pattern>
          '*'-ended file path pattern, where the tool will try to find journal
          and data files
       --journal-file         <journal file>
          path to a .bmq_journal file
       --data-file            <data file>
          path to a .bmq_data file
       --csl-file             <csl file>
          path to a .bmq_csl file
       --guid                 <guid>
          message guid
       --queue-name           <queue name>
          message queue name
       --queue-key            <queue key>
          message queue key
       --timestamp-gt         <timestamp greater than>
          lower timestamp bound
       --timestamp-lt         <timestamp less than>
          higher timestamp bound
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
       --summary
          summary of all matching messages (number of outstanding messages and
          other statistics)
  -h | --help
          print usage
```

Scenarios of BMQStorageTool usage
=================================

Output summary for journal file
----------------------------------------
Example:
```bash
bmqstoragetool.tsk --journal-file=<path> --summary
```

Search and otput all message GUIDs in journal file
--------------------------------------------------
Example:
```bash
bmqstoragetool.tsk --journal-file=<path>
```

Search and otput all messages details in journal file
-----------------------------------------------------
Example:
```bash
bmqstoragetool.tsk --journal-file=<path> --details
```

Search and otput all outstanding/confirmed/partially-confirmed message GUIDs in journal file
--------------------------------------------------------------------------------------------
Example:
```bash
bmqstoragetool.tsk --journal-file=<path> --outstanding
bmqstoragetool.tsk --journal-file=<path> --confirmed 
bmqstoragetool.tsk --journal-file=<path> --partially-confirmed 
```

Search all message GUIDs with payload dump in journal file
----------------------------------------------------------------------
```bash
bmqstoragetool.tsk --journal-file=<journal-path> --data-file=<data-path> --dump-payload
bmqstoragetool.tsk --journal-path=<path.*> --dump-payload
bmqstoragetool.tsk --journal-path=<path.*> --dump-payload --payload-limit=64
```

Applying search filters to above scenarios
==========================================

Filter messages with corresponding GUIDs
----------------------------------------
Example:
```bash
bmqstoragetool.tsk --journal-file=<path> --guid=<guid_1> --guid=<guid_N>
```
NOTE: no other filters are allowed with this one

Filter messages within time range
---------------------------------
Example:
```bash
bmqstoragetool.tsk --journal-file=<path> --timestamp-lt=<stamp>
bmqstoragetool.tsk --journal-file=<path> --timestamp-gt=<stamp>
bmqstoragetool.tsk --journal-file=<path> --timestamp-lt=<stamp1> --timestamp-gt=<stamp2>
```

Filter messages by queue key
----------------------------
Example:
```bash
bmqstoragetool.tsk --journal-file=<path> --queue-key=<key_1> --queue-key=<key_N>
```

Filter messages by queue Uri
----------------------------
Example:
```bash
bmqstoragetool.tsk --journal-file=<journal_path> --csl-file=<csl_path> --queue-name=<queue_uri_1> --queue-name=<queue_uri_N>
```
NOTE: CSL file is required
