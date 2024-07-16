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

import abc
from pathlib import Path
from typing import Union


class Site(abc.ABC):
    @abc.abstractmethod
    def __str__(self) -> str: ...

    @abc.abstractmethod
    def install(self, from_path: str, to_path: str) -> None: ...

    @abc.abstractmethod
    def create_file(self, path: str, content: str, mode=None) -> None: ...

    @abc.abstractmethod
    def mkdir(self, path: str) -> None: ...

    @abc.abstractmethod
    def rmdir(self, path: str) -> None: ...
