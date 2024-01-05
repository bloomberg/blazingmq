"""BMQ Integration Tests


PURPOSE: Provide integration tests and components.

The 'bmqit' package group provides integration tests and attendant modules.
It is primarily used to test specified piece involved in BMQ
infrastructure.

Hierarchical Synopsis
---------------------

The 'bmqit' package group currently contains two packages having two levels
of physical dependency. The list below shows the hierarchical ordering of
the packages.

    2. blazingmq.dev.it.fixtures

    1. blazingmq.dev.it.process

Package Overview
----------------

This section is a brief introduction to the packages of the 'bit' package
group. See the respective package level documents for more details.

blazingmq.dev.it.fixtures
- - - - - - - - - -

This package provides fixture classes that various configurations of nodes,
proxies and datacenters.

blazingmq.dev.it.process
- - - - - - - - -

This package provides classes that encapsulate a process running a broker or a
client.

"""
