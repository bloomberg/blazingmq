"""
Provide a cache of paths (to build dir, to broker, etc), derived from this
module's path, with possible overrides via environment variables.

The module publishes two attributes, 'paths' and 'required_paths'. The latter
raises an exception if queried for a path that does not exist. Both objects
have the following Path attributes:

- repository:
    the repository containing this module
- python_path:
    this repository's Python directory
- build_dir:
    the build area; the value of env var BLAZINGMQ_BUILD_DIR, or
    f"cmake.bld/{platform.system()}" if not set
- broker_path:
    the broker; the value of env var BLAZINGMQ_BROKER, or
    f"{build_dir}/src/applications/bmqbrkr/bmqbrkr.tsk" if not set
- tool_path:
    bmqtool; the value of env var BLAZINGMQ_TOOL, or
    f"{build_dir}/src/applications/bmqtool/bmqtool.tsk" if not set
- plugins_path:
    the value of env var BLAZINGMQ_PLUGINS, or
    f"{build_dir}/src/applications/bmqtool/bmqtool.tsk" if not set
"""

from dataclasses import dataclass
import os
import platform
from pathlib import Path
from typing import Optional

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
    _broker_path: Optional[Path] = None
    _tool_path: Optional[Path] = None
    _plugin_path: Optional[Path] = None

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
    def python_path(self) -> Path:
        """
        Return the path to the Python root directory.
        """

        return self.repository.joinpath("src", "python")

    @property
    def build_dir(self) -> Path:
        """
        Return the path to the build directory.

        Use the BLAZINGMQ_BUILD_DIR environment variable if it is set, otherwise
        assume that the build area is cmake.bld/<arch> in the repository.
        """

        if self._build_dir is not None:
            return self._build_dir

        if path_str := os.environ.get("BLAZINGMQ_BUILD_DIR"):
            self._build_dir = Path(path_str)
        else:
            self._build_dir = self.repository.joinpath("cmake.bld", platform.system())

        if self._build_dir.is_dir():
            logger.debug("build directory is %s", self._build_dir)
        else:
            logger.warning(
                "%s does not exist, or it is not a directory", self._build_dir
            )

        return self._build_dir

    @property
    def broker_path(self) -> Path:
        """
        Return the path to the broker task.
        """

        if self._broker_path is not None:
            return self._broker_path

        if path_str := os.environ.get("BLAZINGMQ_BROKER"):
            self._broker_path = Path(path_str)
        else:
            self._broker_path = self.build_dir.joinpath(
                *"src/applications/bmqbrkr/bmqbrkr.tsk".split("/")
            )

        if not self._broker_path.exists():
            logger.warning("path %s does not exist", self._broker_path)
            if self.must_exist:
                raise FileNotFoundError(self._broker_path)

        return self._broker_path

    @property
    def tool_path(self) -> Path:
        """
        Return the path to the bmqtool task.
        """

        if self._tool_path is not None:
            return self._tool_path

        if path_str := os.environ.get("BLAZINGMQ_TOOL"):
            self._tool_path = Path(path_str)
        else:
            self._tool_path = self.build_dir.joinpath(
                *"src/applications/bmqtool/bmqtool.tsk".split("/")
            )

        if not self._tool_path.exists():
            logger.warning("path %s does not exist", self._tool_path)
            if self.must_exist:
                raise FileNotFoundError(self._tool_path)

        return self._tool_path

    @property
    def plugins_path(self) -> Path:
        """
        Return the path to the plugins directory.
        """

        if self._plugin_path is not None:
            return self._plugin_path

        if path_str := os.environ.get("BLAZINGMQ_PLUGINS"):
            self._plugin_path = Path(path_str)
        else:
            self._plugin_path = self.build_dir.joinpath("src", "plugins")

        if not self._plugin_path.exists():
            logger.warning("path %s does not exist", self._plugin_path)
            if self.must_exist:
                raise FileNotFoundError(self._plugin_path)

        return self._plugin_path


paths = Paths(must_exist=False)
required_paths = Paths(must_exist=True)
