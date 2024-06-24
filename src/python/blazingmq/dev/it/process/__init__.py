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

"""BMQ Integration Tests

PURPOSE: Provide wrappers for processes.

The 'blazingmq.dev.it.process' package groupprovides classes that encapsulate a process
running a broker or a client.

Hierarchical Synopsis
---------------------

The 'blazingmq.dev.it.process' package group currently contains two packages having one
level of physical dependency. The list below shows the hierarchical ordering of
the packages.

    1. blazingmq.dev.it.process.broker
       blazingmq.dev.it.process.client

Package Overview
----------------

This section is a brief introduction to the packages of the 'blazingmq.dev.it.process'
package group. See the respective package level documents for more details.

blazingmq.dev.it.process.broker
- - - - - - - - - -

This package provides a class that encapsulates a process running a broker.

blazingmq.dev.it.process.client
- - - - - - - - - -

This package provides a class that encapsulates a process running a client
(currently bmqtool).

"""
