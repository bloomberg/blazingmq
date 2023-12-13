"""
Provide an object - 'paths' - that contains various paths useful to development
tools (integration tests, CapMon, etc).
"""

import os
import platform
import sys
from pathlib import Path
from typing import Optional

import logging

from bmq.core import BMQError

logger = logging.getLogger(__name__)


class Paths:
    """
    Container for paths to repository root, build area, etc.

    These paths are somewhat costly to calculate so we want to do that only
    once, and only if needed. Unfortunately, Python does not support *module*
    properties, which would be the natural way of achieving this. Instead, we
    use an instance of this class.
    """

    _repository: Optional[Path] = None
    _build_dir: Optional[Path] = None

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
                "build directory %s does not exist, or it is not a directory",
                self._build_dir,
            )

        return self._build_dir

    @property
    def broker_path(self) -> Path:
        """
        Return the path to the broker task.
        """

        return self.build_dir.joinpath(
            *"src/applications/bmqbrkr/bmqbrkr.tsk".split("/")
        )

    @property
    def tool_path(self) -> Path:
        """
        Return the path to the bmqtool task.
        """

        return self.build_dir.joinpath(
            *"src/applications/bmqtool/bmqtool.tsk".split("/")
        )

    @property
    def plugins_path(self) -> Path:
        """
        Return the path to the plugins directory.
        """

        return self.build_dir.joinpath("src", "plugins")


paths = Paths()
