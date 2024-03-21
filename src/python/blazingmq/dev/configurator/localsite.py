from pathlib import Path
from shutil import rmtree
from typing import Dict, Tuple, Union

from blazingmq.dev.configurator.site import Site



class LocalSite(Site):
    def __init__(self, root_dir: Union[Path, str]):
        self.root_dir = Path(root_dir)

    def __str__(self) -> str:
        return str(self.root_dir)

    def mkdir(self, path: Union[str, Path]) -> None:
        target = self.root_dir / path
        target.mkdir(0o755, exist_ok=True, parents=True)

    def rmdir(self, path: Union[str, Path]) -> None:
        rmtree(self.root_dir / path, ignore_errors=True)

    def install(self, from_path: Union[str, Path], to_path: Union[str, Path]) -> None:
        from_path = Path(from_path).resolve()
        to_path = self.root_dir / to_path
        to_path.mkdir(0o755, exist_ok=True, parents=True)
        target = Path(to_path) / from_path.name
        if target.is_symlink():
            target.unlink(missing_ok=True)
        target.symlink_to(from_path.resolve(), target_is_directory=from_path.is_dir())

    def create_file(self, path: Union[str, Path], content: str, mode=None) -> None:
        path = self.root_dir / path
        path.parent.mkdir(0o755, exist_ok=True, parents=True)
        with open(path, "w", encoding="ascii") as out:
            out.write(content)
        path.chmod(mode or 0o644)
