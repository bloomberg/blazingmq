BMQTool
=======

BMQTool is a command-line tool for interfacing with message queues through the
BlazingMQ Broker (sending, reading, and acknowledging messages), and the
interfacing with the files created as a result of running the broker and any
consumers, producers, or workers.  The tool can be found under your `CMAKE`
build directory after making the project.  From the command-line, there are a
few options you can use when invoking the tool.

```bash
Usage: bmqtool [--mode <mode>]
               [-b|broker <address>]
               [-q|queueuri <uri>]
               [-f|queueflags <flags>]
               [-l|latency <latency>]
               [--latency-report <report.json>]
               [-d|dumpmsg]
               [-c|confirmmsg]
               [-e|eventsize <evtSize>]
               [-m|msgsize <msgSize>]
               [-r|postrate <rate>]
               [--eventscount <events>]
               [-u|maxunconfirmed <unconfirmed>]
               [-i|postinterval <interval>]
               [-v|verbosity <verbosity>]
               [--logFormat <logFormat>]
               [-D|memorydebug]
               [-t|threads <threads>]
               [--shutdownGrace <shutdownGrace>]
               [--nosessioneventhandler]
               [-s|storage <storage>]
               [--log <log>]
               [--profile <profile>]
               [-h|help]
               [--dumpprofile]
               [--messagepattern <sequential message pattern>]
               [--messageProperties <MessageProperties>]
               [--subscriptions <Subscriptions>]
Where:
       --mode                   <mode>
          mode ([<cli>, auto, storage, syschk])
  -b | --broker                 <address>
          address and port of the broker (default: tcp://localhost:30114)
  -q | --queueuri               <uri>
          URI of the queue (for auto/syschk modes) (default: )
  -f | --queueflags             <flags>
          flags of the queue (for auto mode) (default: )
  -l | --latency                <latency>
          method to use for latency computation ([<none>, hires, epoch])
          (default: none)
       --latency-report         <report.json>
          where to generate the JSON latency report (default: )
  -d | --dumpmsg
          dump received message content
  -c | --confirmmsg
          confirm received message
  -e | --eventsize              <evtSize>
          number of messages per event (default: 1)
  -m | --msgsize                <msgSize>
          payload size of posted messages in bytes (default: 1024)
  -r | --postrate               <rate>
          number of events to post at each 'postinterval' (default: 1)
       --eventscount            <events>
          If there is an 's' or 'S' at the end, duration in seconds to post;
          otherwise, number of events to post (0 for unlimited) (default: 0)
  -u | --maxunconfirmed         <unconfirmed>
          Maximum unconfirmed msgs and bytes (fmt: 'msgs:bytes') (default:
          1024:33554432)
  -i | --postinterval           <interval>
          interval to wait between each post (default: 1000)
  -v | --verbosity              <verbosity>
          verbosity ([silent, trace, debug, <info>, warning, error, fatal])
          (default: info)
       --logFormat              <logFormat>
          log format (default: %d (%t) %s %F:%l %m )
  -D | --memorydebug
          use a test allocator to debug memory
  -t | --threads                <threads>
          number of processing threads to use
       --shutdownGrace          <shutdownGrace>
          seconds of inactivity (no message posted in auto producer mode, or no
          message received in auto consumer mode) before shutting down
       --nosessioneventhandler
          use custom event handler threads
  -s | --storage                <storage>
          path to storage files to open (default: )
       --log                    <log>
          path to log file (default: )
       --profile                <profile>
          path to profile in JSON format (default: )
  -h | --help
          show the help message
       --dumpprofile
          Dump the JSON profile of the parameters and exit
       --messagepattern         <sequential message pattern>
          printf-like format for sequential message payload content
       --messageProperties      <MessageProperties>
          MessageProperties
       --subscriptions          <Subscriptions>
          Subscriptions
```

Regular Mode
------------

The following is a list of general commands available to the BMQTool when
opened regularly (without specifying a mode).  To run the `bmqtool`, you invoke
`bmqtool`.

| Command   | Arguments      | Description                                                  |
| :-------- | :------------- | :----------------------------------------------------------- |
| `start`   | `[async=true]` | Start a session and event queue (optionally asynchronous).   |
| `stop`    | `[async=true]` | Stop the current session (optionally asynchronous).          |
| `open`    | `uri=string [async=true] [maxUnconfirmedMessages=N) (maxUnconfirmedByes=M)` | Open a connection with the queue at the given `uri`. |
| `close`   | `uri=string [async=true]` | Close connection with the queue at the given `uri`. |
| `post`    | `uri=string payload=string [, ...] [async=true]` | Post a message (the `payload`) to the queue at the given `uri`. |
| `list`    | N/A            | List the messages that have yet to be ACKed by the tool.     |
| `confirm` | `guid=string`  | Confirm (ACK) the message matching the given GUID.           |
| `help`    | N/A            | Show the help dialog.                                        |
| `bye`     | N/A            | Exit the tool.                                               |
| `quit`    | N/A            | Exit the tool.                                               |
| `q`       | N/A            | Exit the tool.                                               |

DataFile Commands
-----------------

The following commands are only available when a data file is open (via the
`open` command).  These commands are prefixed by the letter `d`, so to invoke
one of these commands, you would run it as `d <command> <arg1> ....`;
effectively these are sub-commands having to deal with data file(s).

| Sub-Command | Argument             | Description                           |
| :---------- | :------------------- | :------------------------------------ |
| `n/next`    | `nonNegativeInteger` | Iterate `k` records forward in the file, where `k` is the passed arg. |
| `p/prev`    | `nonNegativeInteger` | Iterate `k` records backwards in the file, where `k` is the passed arg. |
| `r/record`  | `nonNegativeInteger` | Iterate to the `n`th record in the data file. |
| `l/list`    | `int`                | List the next `k` records in the file where `k` is positive.  If `k` is negative, list the `-1 * k` previous records in the file. |

Qlist Commands
--------------

The following commands are only available when a qlist file is open (via the
`open` command).  These commands are prefixed by the letter `q`, so to invoke
one of these commands, you would run it as `q <command> <arg1> ....`;
effectively these are sub-commands having to deal with qlist file(s).

| Sub-Command | Argument             | Description                           |
| :---------- | :------------------- | :------------------------------------ |
| `n/next`    | `nonNegativeInteger` | Iterate `k` records forward in the file, where `k` is the passed arg. |
| `p/prev`    | `nonNegativeInteger` | Iterate `k` records backwards in the file, where `k` is the passed arg. |
| `r/record`  | `nonNegativeInteger` | Iterate to the `n`th record in the qlist file. |
| `l/list`    | `int`                | List the next `k` records in the file where `k` is positive.  If `k` is negative, list the `-1 * k` previous records in the file. |

Journal Commands
----------------

The following commands are only available when a journal file is open (via the
`open` command).  These commands are prefixed by the letter `j`, so to invoke
one of these commands, you would run it as `j <command> <arg1> ...`;
effectively these are sub-commands having to deal with journal file(s).

| Sub-Command | Argument             | Description                           |
| :---------- | :------------------- | :------------------------------------ |
| `n/next`    | `nonNegativeInteger` | Iterate `k` records forward in the file, where `k` is the passed arg. |
| `p/prev`    | `nonNegativeInteger` | Iterate `k` records backwards in the file, where `k` is the passed arg. |
| `r/record`  | `nonNegativeInteger` | Iterate to the `n`th record in the journal file. |
| `l/list`    | `int`                | List the next `k` records in the file where `k` is positive.  If `k` is negative, list the `-1 * k` previous records in the file. |
| `type`      | `{"message", "confirm", "delete", "qop", "jop"}` | Iterate to the next record in the file that matches the given type. |
| `dump`      | `"payload"`          | Dump the payload of the message pointed to by the current record pointed to by the journal iterator (provided it is a message record).  Note that this requires the associated data file to be open. |
