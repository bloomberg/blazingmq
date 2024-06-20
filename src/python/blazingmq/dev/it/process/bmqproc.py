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

from blazingmq.dev.it.process.proc import Process

import blazingmq.dev.it.logging

import time
import logging
import re

logger = logging.getLogger(__name__)


_LOG_FORMAT_DEFINITION = [
    ("%c", r"(\w+(?:\.\w+)*)"),  # category
    # timestamp => datetime, millis
    ("%d", r"(\w{9}_\d{2}:\d{2}:\d{2})\.(\d{3})"),
    ("%t", r"(\d+)"),  # thread
    ("%s", r"([A-Z]+)"),  # level
    ("%F:%l", r"(\w+(?:\.\w+)+):(\d+)"),  # file, line
    ("%m", r"(.*)"),  # message
]

_LOG_FIELD_SEP = "~@~"

PROC_LOG_FORMAT = (
    _LOG_FIELD_SEP.join([specifier for specifier, regex in _LOG_FORMAT_DEFINITION])
    + r"\n"
)

_LOG_LINE_REGEX = re.compile(
    _LOG_FIELD_SEP.join([regex for specifier, regex in _LOG_FORMAT_DEFINITION])
)


class BMQProcess(Process):
    def __init__(self, name, *args, **kwargs):
        self._process_log_category = kwargs.pop("process_log_category")
        super().__init__(name, *args, **kwargs)
        # The following contain values extracted from the last properly
        # formatted record.
        self._last_stdout_log_level = None
        self._last_stdout_log_category = None
        self._last_stdout_log_overrides = None

    def log_stdout(self, record):
        self._internal_logger.log(logging.TRACE, 'parsing stdout: "%60s..."', record)
        parsed = _LOG_LINE_REGEX.match(record)

        if parsed:
            self._internal_logger.log(logging.TRACE, "it is a BALL record")
            # A properly formatted record, i.e. one that contains a level, a
            # category, etc.
            (
                category,
                datetime,
                ms,
                thread,
                level,
                filename,
                line,
                message,
            ) = parsed.groups()
            category = category.lower()
            thread = int(thread)
            ms = int(ms)
            category = f"blazingmq.tsk.{self._process_log_category}.{category}"
            self._last_stdout_log_category = category
            timestamp = (
                time.mktime(time.strptime(datetime, "%d%b%Y_%H:%M:%S")) + ms / 1000.0
            )
            record = message
            self._last_stdout_log_overrides = {
                "created": timestamp,
                "filename": filename,
                "lineno": int(line),
                "thread": int(thread),
            }
            if not hasattr(logging, level):
                logger.warning("invalid log level %s", level)
                level = self._last_stdout_log_level
            else:
                level = getattr(logging, level)
                self._last_stdout_log_level = level
        else:
            # Re-use the last seen level and category - can be None
            category = self._last_stdout_log_category
            level = self._last_stdout_log_level
            self._internal_logger.log(
                logging.TRACE,
                "it is *not* a BALL record, reuse category (%s) and level (%s)"
                " from previous record",
                category,
                logging.getLevelName(level),
            )

        if level is None:
            # This is the output of the process to stdout *before* it started
            # logging.
            category = f"blazingmq.tsk.{self._process_log_category}"
            level = logging.INFO
            self._last_stdout_log_overrides = {
                "filename": "?",
                "lineno": 0,
                "thread": int(0),
            }

        logging.getLogger(category).log(
            level,
            record,
            extra={
                "bmqprocess": self.name,
                "ball_overrides": self._last_stdout_log_overrides,
            },
        )

    def log_stderr(self, record):
        category = f"blazingmq.tsk.{self._process_log_category}.stderr"
        logging.getLogger(category).info("%s", record, extra={"bmqprocess": self.name})
