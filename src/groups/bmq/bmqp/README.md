BMQP
====
> The `BMQP` (BlazingMQ Protocol) package provides BlazingMQ protocol
> definition, builders and parsers.


Description
-----------
The 'bmqp' package provides the definition of the BlazingMQ protocol as well as
the utilities to manipulate the protocol objects.

Note that this package is **INTERNAL**.  Clients are not supposed to use any of
its components directly, but rather use the accessor public APIs from 'bmqa'.


Component Synopsis
------------------
Component                        | Provides ...
---------------------------------|------------------------------------------------------------
`bmqp_ackeventbuilder`           | a mechanism to build a BlazingMQ 'ACK' event.
`bmqp_ackmessageiterator`        | a mechanism to iterate over the messages of an AckEvent.
`bmqp_confirmeventbuilder`       | a mechanism to build a BlazingMQ 'CONFIRM' event.
`bmqp_confirmmessageiterator`    | a mechanism to iterate over messages of a 'CONFIRM' event.
`bmqp_controlmessageutil`        | utilities related to control messages.
`bmqp_crc32c`                    | utilities for calculating crc32c.
`bmqp_ctrlmsg_messages`          | value-semantic types for control messages.
`bmqp_eventutil`                 | utilities for BlazingMQ protocol events.
`bmqp_event`                     | a mechanism to access messages from a raw BlazingMQ event packet.
`bmqp_messageproperties`         | a VST representing message properties.
`bmqp_optionsview`               | a mechanism to access options of a message.
`bmqp_optionutil`                | utilities for builders and iterators that use options.
`bmqp_protocol`                  | definitions for BlazingMQ protocol structures and constants.
`bmqp_protocolutil`              | utilities for BlazingMQ protocol builders and iterators.
`bmqp_pusheventbuilder`          | a mechanism to build a BlazingMQ 'PUSH' event.
`bmqp_pushmessageiterator`       | a mechanism to iterate over messages of a 'PUSH' event.
`bmqp_puteventbuilder`           | a mechanism to build a BlazingMQ 'PUT' event.
`bmqp_putmessageiterator`        | a mechanism to iterate over messages of a 'PUT' event.
`bmqp_puttester`                 | utility methods for testing 'PUT' events.
`bmqp_queueid`                   | a value-semantic type for a key related to a queue.
`bmqp_recoveryeventbuilder`      | a mechanism to build a BlazingMQ 'RECOVERY' event.
`bmqp_recoverymessageiterator`   | a mechanism to iterate over messages of a 'RECOVERY' event.
`bmqp_rejecteventbuilder`        | a mechanism to build a BlazingMQ 'REJECT' event.
`bmqp_rejectmessageiterator`     | a mechanism to iterate over messages of a 'REJECT' event.
`bmqp_requestmanager`            | a mechanism to manipulate requests and their response.
`bmqp_routingconfigurationutils` | utility methods to manage 'RoutingConfiguration' instances.
`bmqp_schemaeventbuilder`        | a mechanism to build a BlazingMQ schema event.
`bmqp_storageeventbuilder`       | a mechanism to build a BlazingMQ  'STORAGE' event.
`bmqp_storagemessageiterator`    | a mechanism to iterate over messages of a 'STORAGE' event.
