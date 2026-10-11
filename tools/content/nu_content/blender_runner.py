from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path

from nu_content.paths import blender_scripts_dir


def find_blender() -> str:
    env = os.environ.get("NU_BLENDER")
    if env and Path(env).exists():
        return env
    which = shutil.which("blender")
    if which:
        return which
    # Common Windows installs
    candidates = [
        Path(r"C:\Program Files\Blender Foundation\Blender 4.2\blender.exe"),
        Path(r"C:\Program Files\Blender Foundation\Blender 4.3\blender.exe"),
        Path(r"C:\Program Files\Blender Foundation\Blender 4.1\blender.exe"),
    ]
    for c in candidates:
        if c.exists():
            return str(c)
    raise FileNotFoundError(
        "Blender not found. Install Blender LTS 4.2 and set NU_BLENDER to blender.exe"
    )


def run_blender_script(script_name: str, *args: str, blend_file: Path | None = None) -> None:
    blender = find_blender()
    script = blender_scripts_dir() / script_name
    if not script.exists():
        raise FileNotFoundError(f"Missing Blender script: {script}")

    cmd = [blender, "--background"]
    if blend_file and blend_file.exists():
        cmd.append(str(blend_file))
    cmd += ["--python", str(script), "--", *args]

    proc = subprocess.run(cmd, check=False, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"Blender failed ({proc.returncode}):\nSTDOUT:\n{proc.stdout}\nSTDERR:\n{proc.stderr}"
        )
