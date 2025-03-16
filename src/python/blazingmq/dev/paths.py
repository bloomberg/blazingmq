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
Provide a cache of paths (to build dir, to broker, etc), derived from this
module's path, with possible overrides via environment variables.

The module publishes two attributes, 'paths' and 'required_paths'. The latter
raises an exception if queried for a path that does not exist. Both objects
have the following Path attributes:

- repository:
    the repository containing this module
- python:
    this repository's Python directory
- build_dir:
    the build area; the value of env var BLAZINGMQ_BUILD_DIR, or
    f"cmake.bld/{platform.system()}" if not set
- broker:
    the broker; the value of env var BLAZINGMQ_BROKER, or
    f"{build_dir}/src/applications/bmqbrkr/bmqbrkr.tsk" if not set
- tool:
    bmqtool; the value of env var BLAZINGMQ_TOOL, or
    f"{build_dir}/src/applications/bmqtool/bmqtool.tsk" if not set
- plugins:
    the value of env var BLAZINGMQ_PLUGINS, or
    f"{build_dir}/src/applications/bmqtool/bmqtool.tsk" if not set
"""

from dataclasses import dataclass
import os
import platform
from pathlib import Path
from typing import Optional
import random

import logging

from blazingmq.core import BMQError

logger = logging.getLogger(__name__)


@dataclass
class Paths:
    """
    Container for paths to repository root, build area, etc.

    These paths are somewhat costly to calculate so we want to do that only
    once, and only if needed. Unfortunately, Python does not support *module*
    properties, which would be the natural way of achieving this. Instead, we
    use an instance of this class.
    """

    must_exist: bool

    _repository: Optional[Path] = None
    _build_dir: Optional[Path] = None
    _broker: Optional[Path] = None
    _tool: Optional[Path] = None
    _plugin: Optional[Path] = None

    @property
    def repository(self) -> Path:
        """
        Return the path to the OSS repository. By default, it assumes OSS repo
        is on the same directory level next to the enterprise repo.
        """

        if self._repository is not None:
            return self._repository

        self._repository = Path(__file__)
        while not (self._repository / "src").exists():
            if self._repository == self._repository.parent:
                raise BMQError("cannot locate blazingmq repository")
            self._repository = self._repository.parent

        logger.debug("blazingmq repository is %s", self._repository)

        if not self._repository.is_dir():
            logger.error("blazingmq directory does not exist, or it is not a directory")

        logger.debug("blazingmq repository is %s", self._repository)

        return self._repository

    @property
    def python(self) -> Path:
        """
        Return the path to the Python root directory.
        """

        return self.repository.joinpath("src", "python")

    @property
    def build_dir(self) -> Path:
        """
        Return the path to the build directory.

        Use the BLAZINGMQ_BUILD_DIR environment variable if it is set, otherwise
        assume that the build area is build/blazingmq in the repository.
        """

        if self._build_dir is not None:
            return self._build_dir

        if path_str := os.environ.get("BLAZINGMQ_BUILD_DIR"):
            self._build_dir = Path(path_str)
        else:
            self._build_dir = self.repository.joinpath("build/blazingmq")

        if self._build_dir.is_dir():
            logger.debug("build directory is %s", self._build_dir)
        else:
            logger.warning(
                "%s does not exist, or it is not a directory", self._build_dir
            )

        return self._build_dir

    @property
    def broker(self) -> Path:
        """
        Return the path to the broker task.
        """

        if self._broker is not None:
            return self._broker

        if path_str := os.environ.get("BLAZINGMQ_BROKER"):
            self._broker = Path(path_str)
        else:
            self._broker = self.build_dir.joinpath(
                *"src/applications/bmqbrkr/bmqbrkr.tsk".split("/")
            )

        if not self._broker.exists():
            logger.warning("path %s does not exist", self._broker)
            if self.must_exist:
                raise FileNotFoundError(self._broker)

        return self._broker

    def get_broker_path(self, name: str) -> Path:
        path_str = os.environ.get(f"BLAZINGMQ_BROKER_{name.upper()}")

        if not path_str:
            return self.broker

        brokerPath = Path(path_str)

        if not brokerPath.exists():
            return self.broker

        return brokerPath

    @property
    def tool(self) -> Path:
        """
        Return the path to the bmqtool task.
        """

        if self._tool is not None:
            return self._tool

        if path_str := os.environ.get("BLAZINGMQ_TOOL"):
            self._tool = Path(path_str)
        else:
            self._tool = self.build_dir.joinpath(
                *"src/applications/bmqtool/bmqtool.tsk".split("/")
            )

        if not self._tool.exists():
            logger.warning("path %s does not exist", self._tool)
            if self.must_exist:
                raise FileNotFoundError(self._tool)

        return self._tool

    @property
    def plugins(self) -> Path:
        """
        Return the path to the plugins directory.
        """

        if self._plugin is not None:
            return self._plugin

        if path_str := os.environ.get("BLAZINGMQ_PLUGINS"):
            self._plugin = Path(path_str)
        else:
            self._plugin = self.build_dir.joinpath("src", "plugins")

        if not self._plugin.exists():
            logger.warning("path %s does not exist", self._plugin)
            if self.must_exist:
                raise FileNotFoundError(self._plugin)

        return self._plugin


paths = Paths(must_exist=False)
required_paths = Paths(must_exist=True)
