"""Remove only the disposable ESP-IDF build workspace after a Builder job.

The validated firmware package and private device provisioning are copied to
/data/releases before cleanup. A new build already recreates its workspace
from scratch, so retaining it only wastes disk space and Linux file cache.
"""
from __future__ import annotations

import shutil
from pathlib import Path


def discard_build_workspace(data_root: Path) -> bool:
    """Remove the fixed disposable workspace, never releases or toolchain.

    Refuse unexpected paths such as symlinks, rather than following them.
    Returns False if the workspace has already been removed.
    """
    workspace = Path(data_root) / "build_workspace"
    if workspace.is_symlink():
        raise OSError("Refusing to remove a symlinked build workspace")
    if not workspace.exists():
        return False
    if not workspace.is_dir():
        raise OSError("Build workspace is not a directory")
    shutil.rmtree(workspace)
    return True
