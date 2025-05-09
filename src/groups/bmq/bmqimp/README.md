BMQIMP
======
> The `BMQIMP` (BlazingMQ (API) Implementation) package provides implementation
> for the API of the BlazingMQ SDK.


Description
-----------
The 'bmqpimp' package provides the implementation API of the BlazingMQ SDK.

Note that this package is **INTERNAL**.  Clients are not supposed to use any of
its components directly, but rather use the accessor public APIs from 'bmqa'.

Component Synopsis
------------------
Component                                | Provides ...
-----------------------------------------|-----------------------------------------------------------
`bmqimp_application`                     | the top level object to manipulate a session with bmqbrkr.
`bmqimp_brokersession`                   | the implementation of a session with the bmqbrkr.
`bmqimp_event`                           | a value-semantic type representing an event.
`bmqimp_eventqueue`                      | a thread safe queue of pooled bmqimp::Event items.
`bmqpimp_eventsstats`                    | a mechanism to keep track of Events statistics.
`bmqpimp_queue`                          | a type object to represent information about a queue.
`bmqimp_stat`                            | utilities for stat manipulation.
`bmqimp_initialconnectionchannelfactory` | a channel factory that authenticates a peer and negotiates with it.
