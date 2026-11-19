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

"""
Support for creating a "configurator", i.e. a collection of directories and
scripts for running a cluster.
"""

# mypy: disable-error-code="union-attr"
# pylint: disable=missing-function-docstring, missing-class-docstring, consider-using-f-string
# pyright: reportOptionalMemberAccess=false

import contextlib
import itertools
import logging
import subprocess
import threading
from dataclasses import dataclass, field
from pathlib import Path
from typing import IO, Dict, Optional

from termcolor import colored

from blazingmq.dev.configurator.configurator import Broker, Configurator

COLORS = {
    "green": 32,
    "yellow": 33,
    "magenta": 35,
    "cyan": 36,
    "blue": 34,
    "light_green": 92,
    "light_yellow": 93,
    "light_blue": 94,
    "light_magenta": 95,
    "light_cyan": 96,
}


logger = logging.getLogger(__name__)
broker_logger = logger.getChild("broker")


class MonitoredProcess:
    process: Optional[subprocess.Popen] = None
    thread: Optional[threading.Thread] = None


def broker_monitor(out: IO[str], prefix: str, color: str):
    while not out.closed:
        line = out.readline()
        if line == "":
            break
        line = line.rstrip(" \n\r")
        if line:
            broker_logger.info(colored("%s | %s", color), prefix, line)


@dataclass
class Session(contextlib.AbstractContextManager):
    configurator: Configurator
    root: Path
    brokers: Dict[Broker, MonitoredProcess] = field(default_factory=dict)

    def __exit__(self, *args):
        for broker in reversed(self.brokers.values()):
            if broker.process is not None:
                broker.process.__exit__(*args)

        for broker in reversed(self.brokers.values()):
            if broker.thread is not None:
                broker.thread.join()

    def stop(self):
        for broker in self.brokers.values():
            if broker.process is not None:
                broker.process.terminate()
                broker.process.wait()

    def run(self):
        colors = itertools.cycle(COLORS)
        prefix_len = max(len(name) for name in self.configurator.brokers)

        for broker in self.configurator.brokers.values():
            monitored = MonitoredProcess()
            self.brokers[broker] = monitored

            monitored.process = subprocess.Popen(
                [self.root.joinpath(broker.name, "run")],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                encoding="ASCII",
                bufsize=0,
            )

            assert monitored.process.stdout is not None
            monitored.thread = threading.Thread(
                target=broker_monitor,
                args=(
                    monitored.process.stdout,
                    broker.name.ljust(prefix_len),
                    next(colors),
                ),
            )
            monitored.thread.start()
