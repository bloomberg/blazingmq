"""BMQ Integration Tests

PURPOSE: Provide wrappers for processes.

The 'bmq.dev.it.process' package groupprovides classes that encapsulate a process
running a broker or a client.

Hierarchical Synopsis
---------------------

The 'bmq.dev.it.process' package group currently contains two packages having one
level of physical dependency. The list below shows the hierarchical ordering of
the packages.

    1. bmq.dev.it.process.broker
       bmq.dev.it.process.client

Package Overview
----------------

This section is a brief introduction to the packages of the 'bmq.dev.it.process'
package group. See the respective package level documents for more details.

bmq.dev.it.process.broker
- - - - - - - - - -

This package provides a class that encapsulates a process running a broker.

bmq.dev.it.process.client
- - - - - - - - - -

This package provides a class that encapsulates a process running a client
(currently bmqtool).

"""
