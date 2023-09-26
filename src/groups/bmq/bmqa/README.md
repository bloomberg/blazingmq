BMQA
====
> The `BMQA` (BlazingMQ API) package provides applications a public API for the
> BlazingMQ SDK.


Description
-----------
The `bmqa` package provides the public API of the BlazingMQ SDK for
applications to use.

Component Synopsis
------------------
Component                      | Provides ...
-------------------------------|-----------------------------------------------------------
`bmqa_abstractsession`         | a pure protocol for a BlazingMQ session.
`bmqa_closequeuestatus`        | a value-semantic type for a close queue operation status.
`bmqa_configurequeuestatus`    | a value-semantic type for a configure queue operation status.
`bmqa_confirmeventbuilder`     | a builder for batching confirmation messages.
`bmqa_event`                   | a generic variant encompassing all types of events.
`bmqa_manualhosthealthmonitor` | a minimal implementation of `bmqpi::HostHealthMonitor`.
`bmqa_message`                 | the application with a message data object.
`bmqa_messageevent`            | the application with data event notifications.
`bmqa_messageeventbuilder`     | a builder for `MessageEvent` objects.
`bmqa_messageiterator`         | a mechanism to iterate over the messages of a `MessageEvent`.
`bmqa_messageproperties`       | a value-semantic type representing message properties.
`bmqa_mocksession`             | a mock session, implementing `bmqa::AbstractSession`.
`bmqa_openqueuestatus`         | a value-semantic type for an open queue operation status.
`bmqa_queueid`                 | a value-semantic efficient identifier for a queue.
`bmqa_session`                 | access to the BlazingMQ broker.
`bmqa_sessionevent`            | a value-semantic type for system event session notifications.
