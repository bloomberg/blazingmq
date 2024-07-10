# Copyright 2024 Bloomberg Finance L.P.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""BlazingMQ Integration Tests


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
