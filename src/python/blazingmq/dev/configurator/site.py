import abc
from pathlib import Path
from typing import Union



class Site(abc.ABC):
    @abc.abstractmethod
    def __str__(self) -> str:
        ...

    @abc.abstractmethod
    def install(self, from_path: str, to_path: str) -> None:
        ...

    @abc.abstractmethod
    def create_file(self, path: str, content: str, mode=None) -> None:
        ...

    @abc.abstractmethod
    def mkdir(self, path: str) -> None:
        ...

    @abc.abstractmethod
    def rmdir(self, path: str) -> None:
        ...
